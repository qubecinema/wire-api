/**
 * @file QubeWireClient.cpp
 *
 * @copyright Copyright &copy; 2017 Qube Cinema Inc. All Rights reserved
 *
 * @brief
 * Implementation of QubeWireClient class
 */

#include "QubeWireClient.h"
#include "Certificates.h"

#include <boost/network/include/http/client.hpp>
#include <boost/network/protocol/http/response.hpp>
#include <boost/network/tags.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/algorithm/string.hpp>

#include <sstream>

using namespace QUBE_WIRE_NS;
namespace http = boost::network::http;
namespace uri = boost::network::uri;
namespace ptree = boost::property_tree;
namespace filesystem = boost::filesystem;
using boost::network::header;
using boost::network::body;
using namespace boost::algorithm;
using namespace std;

typedef http::basic_response<http::tags::http_server> http_response;

const string QUBEWIRE_PRODUCT_ID = "07c0e191-c79c-48c2-8d93-43e2a67ef1d0";
const string QUBEWIRE_URL = "https://api.qubewire.com";
const string QUBEACCOUNT_URL = "https://account.qubecinema.com";

struct QubeWireClient::Impl
{
    Impl(const string& clientId)
    {
        _baseUrl = QUBEWIRE_URL + "/v1";
        if (!_baseUrl.is_valid())
        {
            throw runtime_error("Invalid Qube Wire URL!");
        }

        _clientId = clientId;
        _refreshToken = "";
        _accessToken = "";
        _sessionId = "";
        _tokenType = "";
        _certificate = "";

        _InitializeHttpClient();
    }

    ~Impl()
    {
    }

    string GetLoginUrl()
    {
        uri::uri requestUri = QUBEACCOUNT_URL;
        requestUri << uri::path("/dialog/polling/initialize");

        ptree::ptree jsonBody;
        ptree::ptree services;
        ptree::ptree products;
        products.put<string>("", QUBEWIRE_PRODUCT_ID);
        services.push_back(make_pair("", products));
        jsonBody.put_child("services", services);
        jsonBody.put<string>("client_id", _clientId);

        stringstream requestBody;
        ptree::write_json(requestBody, jsonBody);

        http::client::response response =
            _PostRequest(requestUri, requestBody.str(), "application/json");
        if (status(response) != http_response::ok)
        {
            throw runtime_error(_GetErrorMessage(response));
        }

        _sessionId = _ParseJsonProperty(body(response), "code");
        _pollingEndpoint = _ParseJsonProperty(body(response), "polling_url");

        stringstream authenticationUrl;
        authenticationUrl << _ParseJsonProperty(body(response), "authorization_url");

        return authenticationUrl.str();
    }

    bool IsAuthenticated()
    {
        stringstream requestBody;
        requestBody << "code=" << _sessionId << "&client_id=" << _clientId
                    << "&client_secret=null&grant_type=authorization_code&access_type=offline";

        http::client::response response =
            _PostRequest(_pollingEndpoint, requestBody.str(), "application/x-www-form-urlencoded");

        if (status(response) != http_response::accepted && status(response) != http_response::ok)
        {
            throw runtime_error(_GetErrorMessage(response));
        }

        if (status(response) == http_response::accepted)
        {
            return false;
        }

        _refreshToken = _ParseAuthorizeInfo(body(response), "refresh_token");
        if (_refreshToken == "")
        {
            throw runtime_error("Unable to get refresh token");
        }
        _accessToken = _GetAccessToken(_refreshToken);

        return true;
    }

    string GetToken() { return _refreshToken; }

    void ResetToken()
    {
        // We are getting a new token here to make sure token is not expired
        _accessToken = _GetAccessToken(_refreshToken);

        uri::uri requestUri = QUBEACCOUNT_URL;
        requestUri << uri::path("/oauth/token?token=") << uri::path(_accessToken);

        http::client::response response = _DeleteRequest(requestUri);

        // In case of failure: Here purposefully cleared tokens before
        // throwing exception, so user shall sign in again (as if signing
        // in from begining)
        _refreshToken.clear();
        _accessToken.clear();
        _tokenType.clear();
        _certificate.clear();

        if (status(response) != http_response::ok)
        {
            throw runtime_error(_GetErrorMessage(response));
        }
    }

    void GetUserInfo(string& emailId, string& companyName)
    {
        uri::uri requestUri = _baseUrl;
        requestUri << uri::path("/users/me");

        http::client::response response = _GetResponse(requestUri);

        if (status(response) != http_response::ok)
        {
            throw runtime_error(_GetErrorMessage(response));
        }

        emailId = _ParseJsonProperty(body(response), "email");
        companyName = _ParseJsonProperty(body(response), "companyName");
    }

    string GetCertificateChain()
    {
        // Here it returns a new access token which is valid for an hour
        // Also we assume that the following tasks will happen within
        // an hour
        // 1) company certificate
        // 2) CPL signing
        // 3) KDM uploading
        // 4) PKL signing
        _accessToken = _GetAccessToken(_refreshToken);

        try
        {
            if (_certificate == "")
            {
                _certificate = _GetCertificateChain();
            }

            return _certificate;
        }
        catch(...)
        {
          throw runtime_error("Unable to get certificate chain since Qube Wire user has not "
                              "generated any certificate");
        }
    }

    string UploadKdm(const string& kdmXml)
    {
        uri::uri requestUri = _baseUrl;
        requestUri << uri::path("/dkdms");

        http::client::response response = _PostRequest(requestUri, kdmXml, "application/xml");
        if (status(response) != http_response::accepted)
        {
            throw runtime_error(_GetErrorMessage(response));
        }

        return _ParseJsonProperty(body(response), "id");
    }

    string Sign(const string& assetXml)
    {
        uri::uri requestUri = _baseUrl;
        requestUri << uri::path("/signer/jobs");

        http::client::response response = _PostRequest(requestUri, assetXml, "application/xml");
        if (status(response) != http_response::accepted)
        {
            throw runtime_error(_GetErrorMessage(response));
        }

        return _ParseJsonProperty(body(response), "id");
    }

    bool GetSignedAssetXml(const string& assetId, string& signedXmlAsset)
    {
        stringstream requestUriString;
        requestUriString << "/signer/jobs/";
        requestUriString << assetId;

        uri::uri requestUri = _baseUrl;
        requestUri << uri::path(requestUriString.str());

        http::client::response response = _GetResponse(requestUri, "application/xml");
        if (status(response) == http_response::ok)
        {
            signedXmlAsset = body(response);
            return true;
        }
        else if (status(response) == http_response::accepted)
        {
            signedXmlAsset = "";
            return false;
        }

        throw runtime_error(_GetErrorMessage(response));
    }

private:
    void _InitializeHttpClient()
    {
        http::client::options options;

        ostringstream caCerts;
        caCerts << QUBEWIRE_ROOT_CA_PEM;
        caCerts << QUBEACCOUNT_ROOT_CA_PEM;
        options.openssl_certificates_buffer(caCerts.str());
        options.always_verify_peer(true);
        _client.reset(new http::client(options));
    }

    string _GetAuthorizationHeader()
    {
        stringstream authHeader;
        authHeader << _tokenType << " " << _accessToken;

        return authHeader.str();
    }

    http::client::response _GetResponse(const uri::uri& requestUri, const string& contentType = "")
    {
        http::client::request request(requestUri);

        if (!_accessToken.empty())
        {
            request << header("Authorization", _GetAuthorizationHeader());
        }

        if (!contentType.empty())
        {
            request << header("Accept", contentType);
        }

        return _client->get(request);
    }

    http::client::response _PostRequest(const uri::uri& requestUri, const string& requestBody,
                                        const string& contentType)
    {
        http::client::request request(requestUri);

        if (!_accessToken.empty())
        {
            request << header("Authorization", _GetAuthorizationHeader());
        }
        if (!contentType.empty())
        {
            request << header("Content-Type", contentType);
        }

        return _client->post(request, requestBody);
    }

    http::client::response _DeleteRequest(const uri::uri& requestUri)
    {
        http::client::request request(requestUri);

        return _client->delete_(request);
    }

    string _GetAccessToken(const string& refreshToken)
    {
        uri::uri requestUri = QUBEACCOUNT_URL;
        requestUri << uri::path("/oauth/token");

        stringstream requestBody;
        requestBody << "client_id=" <<  _clientId << "&client_secret=null&grant_type=refresh_token"
                    << "&refresh_token=" << refreshToken << "&product_id=" << QUBEWIRE_PRODUCT_ID;

        // Access token should be set to empty before calling following _PostRequest,
        // since we are getting new access token
        _accessToken = "";
        http::client::response response =
            _PostRequest(requestUri, requestBody.str(), "application/x-www-form-urlencoded");

        if (status(response) != http_response::ok)
        {
            throw runtime_error(_GetErrorMessage(response));
        }
        _tokenType = _ParseJsonProperty(body(response), "token_type");

        return _ParseJsonProperty(body(response), "access_token");
    }

    string _ParseAuthorizeInfo(const string& json, const string& propertyName)
    {
        stringstream jsonStream(json);
        ptree::ptree pt;

        ptree::read_json(jsonStream, pt);

        if (pt.empty())
        {
            throw runtime_error("Response body is empty");
        }

        for (ptree::ptree::value_type& val : pt)
        {
            auto companyProperties = val.second;
            try
            {
                if(companyProperties.get<string>("product_id") == QUBEWIRE_PRODUCT_ID)
                    return companyProperties.get<string>(propertyName);
            }
            catch(...)
            {
              stringstream error;
              error << "Parsing ";
              error << propertyName;
              error << " failed.";

              throw runtime_error(error.str().c_str());
            }
        }

        stringstream error;
        error << "Authorization information not available for ";
        error << propertyName;

        throw runtime_error(error.str().c_str());
    }

    static string _GetErrorMessage(http::client::response& response,
                                   const string& propertyName = "message")
    {
        try
        {
            return _ParseJsonProperty(static_cast<string>(body(response)), propertyName);
        }
        catch (const exception&)
        {
            return static_cast<string>(status_message(response));
        }
    }

    static string _ParseJsonProperty(const string& json, const string& propertyName)
    {
        stringstream jsonStream(json);
        ptree::ptree pt;

        ptree::read_json(jsonStream, pt);

        if (pt.empty())
        {
            throw runtime_error("Response body is empty");
        }

        try
        {
            return pt.get<string>(propertyName);
        }
        catch(...)
        {
          stringstream error;
          error << "Parsing ";
          error << propertyName;
          error << " failed.";

          throw runtime_error(error.str().c_str());
        }
    }

    string _GetCertificateChain()
    {
        uri::uri requestUri = _baseUrl;
        requestUri << uri::path("/users/me/companies/");

        http::client::response response = _GetResponse(requestUri);

        if (status(response) != http_response::ok)
        {
            throw runtime_error(_GetErrorMessage(response));
        }


        string isCertGenerated = to_lower_copy(_ParseJsonProperty(body(response), "certificateGenerated"));
        if (isCertGenerated != "true")
        {
            throw runtime_error("User doesn't have any certificates. Certificates need to be added");
        }

        return _ParseJsonProperty(body(response), "certificate");
    }

private:
    unique_ptr<http::client> _client;
    uri::uri _pollingEndpoint;
    uri::uri _baseUrl;

    string _clientId;
    string _sessionId;
    string _accessToken;
    string _refreshToken;
    string _tokenType;
    string _certificate;
};

QubeWireClient::QubeWireClient(const string& clientId)
{
    _impl.reset(new Impl(clientId));
}

QubeWireClient::~QubeWireClient()
{
}

string QubeWireClient::GetLoginUrl()
{
    return _impl->GetLoginUrl();
}

bool QubeWireClient::IsAuthenticated()
{
    return _impl->IsAuthenticated();
}

std::string QubeWireClient::GetToken()
{
    return _impl->GetToken();
}

void QubeWireClient::ResetToken()
{
    _impl->ResetToken();
}

void QubeWireClient::GetUserInfo(string& emailId, string& companyName)
{
    _impl->GetUserInfo(emailId, companyName);
}

string QubeWireClient::GetCertificateChain()
{
    return _impl->GetCertificateChain();
}

string QubeWireClient::UploadKdm(const string& kdmXml)
{
    return _impl->UploadKdm(kdmXml);
}

string QubeWireClient::Sign(const string& assetXml)
{
    return _impl->Sign(assetXml);
}

bool QubeWireClient::GetSignedAssetXml(const string& assetId, string& signedXmlAsset)
{
    return _impl->GetSignedAssetXml(assetId, signedXmlAsset);
}
