/**
 * @file KeySmithClient.cpp
 * @author  Qube Dev <QubeDev@realimage.com>
 *
 * @copyright Copyright &copy; 2015 Qube Cinema Inc. All Rights reserved
 *
 * @brief
 * Implementation of KeySmithClient class
 */

#include "KeySmithClient.h"
#include "KeySmithCertificates.h"

#include <boost/network/include/http/client.hpp>
#include <boost/network/protocol/http/response.hpp>
#include <boost/network/tags.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>

#include <sstream>

using namespace KEY_SMITH_NS;
namespace http = boost::network::http;
namespace uri = boost::network::uri;
namespace ptree = boost::property_tree;
namespace filesystem = boost::filesystem;
using boost::network::header;
using boost::network::body;
using namespace std;

typedef http::basic_response<http::tags::http_server> http_response;

// For readability following const declared, but we shouldn't have any other consts like this
const string AUTH_PENDING_CODE = "AUTH2021";
const string SESSION_EXPIRED = "AUTH4044";
const string USER_DENIED = "AUTH4041";

struct KeySmithClient::Impl
{
    Impl(const string& keySmithUrl, const string& clientId, const std::string& token)
    {
        _keySmithUrl = keySmithUrl;
        _clientId = clientId;
        _refreshToken = token;
        _accessToken = "";
        _sessionId = "";
        _tokenType = "";

        if (!_keySmithUrl.is_valid())
        {
            throw runtime_error("Invalid KeySmith URL!");
        }

        _InitHttpClient();

        // Fetching access token using refresh token from previous sign in session
        try
        {
            if (!token.empty())
            {
                _accessToken = _GetAccessToken(_refreshToken);
            }
        }
        catch (...)
        {
            _refreshToken = "";
            _accessToken = "";
            _tokenType = "";
        }
    }

    ~Impl()
    {
        if (!_refreshToken.empty())
            ResetToken();

        if (filesystem::exists(_ksRootCertFilePath))
        {
            filesystem::remove(_ksRootCertFilePath);
        }
    }

    string GetLoginUrl()
    {
        uri::uri requestUri = _keySmithUrl;
        requestUri << uri::path("/oauth2/authorization/request");

        stringstream requestBody;
        requestBody << "client_id=" << uri::encoded(_clientId.c_str());

        http::client::response response =
            _PostRequest(requestUri, requestBody.str(), "application/x-www-form-urlencoded");
        if (status(response) != http_response::ok)
        {
            throw runtime_error(_GetErrorMessage(response).c_str());
        }

        _sessionId = _ParseJsonProperty(body(response), "code");
        _pollingEndpoint = _ParseJsonProperty(body(response), "token_url");

        stringstream authenticationUrl;
        authenticationUrl << _ParseJsonProperty(body(response), "authorization_url")
                          << "?code=" << _sessionId;
        return authenticationUrl.str();
    }

    bool IsAuthenticated()
    {
        stringstream requestBody;
        requestBody << "client_id=" << uri::encoded(_clientId.c_str())
                    << "&grant_type=" << uri::encoded("authorization_code")
                    << "&code=" << uri::encoded(_sessionId.c_str());

        http::client::response response =
            _PostRequest(_pollingEndpoint, requestBody.str(), "application/x-www-form-urlencoded");

        if (status(response) != http_response::accepted && status(response) != http_response::ok)
        {
            if (status(response) == http_response::not_found)
            {
                if (_ParseJsonProperty(body(response), "code") == USER_DENIED.c_str())
                {
                    throw runtime_error("You have denied access");
                }
                else if (_ParseJsonProperty(body(response), "code") == SESSION_EXPIRED.c_str())
                {
                    throw runtime_error("KeySmith sign in session expired");
                }
            }
            throw runtime_error(_GetErrorMessage(response).c_str());
        }

        if (status(response) == http_response::accepted &&
            _ParseJsonProperty(body(response), "code") == AUTH_PENDING_CODE.c_str())
        {
            return false;
        }

        _accessToken = _ParseJsonProperty(body(response), "access_token");
        _refreshToken = _ParseJsonProperty(body(response), "refresh_token");
        _tokenType = _ParseJsonProperty(body(response), "token_type");

        return true;
    }

    string GetToken()
    {
        return _refreshToken;
    }

    string GetLogoutUrl()
    {
        uri::uri requestUri = _keySmithUrl;
        requestUri << uri::path("/logout");

        return requestUri.string();
    }

    void ResetToken()
    {
        // We are getting a new token here to make sure token is not expired
        _accessToken = _GetAccessToken(_refreshToken);

        uri::uri requestUri = _keySmithUrl;
        requestUri << uri::path("/oauth2/token");

        http::client::response response = _DeleteRequest(requestUri);

        // In case of failure: Here purposefully cleared tokens before
        // throwing exception, so user shall sign in again (as if signing
        // in from begining)
        _refreshToken.clear();
        _accessToken.clear();

        if (status(response) != http_response::ok)
        {
            throw runtime_error(_GetErrorMessage(response).c_str());
        }
    }

    void GetUserInfo(string& emailId, string& companyName)
    {
        uri::uri requestUri = _keySmithUrl;
        requestUri << uri::path("/v1/users/me");

        http::client::response response = _GetResponse(requestUri);

        if (status(response) != http_response::ok)
        {
            throw runtime_error(_GetErrorMessage(response).c_str());
        }

        emailId = _ParseJsonProperty(body(response), "email");

        _GetCompanies();
        companyName = _companies[0].Name;
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

        if (!_companies.size())
        {
            _GetCompanies();
        }

        if (!_companies[0].CertificateGenerated)
        {
            throw runtime_error("Unable to get certificate chain since KeySmith user has not "
                                "generated any certificate");
        }

        return _companies[0].Certificate;
    }

    string UploadKdm(const string& kdmXml)
    {
        uri::uri requestUri = _keySmithUrl;
        requestUri << uri::path("/v1/dkdms");

        http::client::response response = _PostRequest(requestUri, kdmXml, "application/xml");
        if (status(response) != http_response::accepted)
        {
            throw runtime_error(_GetErrorMessage(response).c_str());
        }

        return _ParseJsonProperty(body(response), "id");
    }

    string Sign(const string& assetXml)
    {
        uri::uri requestUri = _keySmithUrl;
        requestUri << uri::path("/v1/signer/jobs");

        http::client::response response = _PostRequest(requestUri, assetXml, "application/xml");
        if (status(response) != http_response::accepted)
        {
            throw runtime_error(_GetErrorMessage(response).c_str());
        }

        return _ParseJsonProperty(body(response), "id");
    }

    bool GetSignedAssetXml(const string& assetId, string& signedXmlAsset)
    {
        stringstream requestUriString;
        requestUriString << "/v1/signer/jobs/";
        requestUriString << assetId;

        uri::uri requestUri = _keySmithUrl;
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

        throw runtime_error(_GetErrorMessage(response).c_str());
    }

private:
    void _InitHttpClient()
    {
        // TO DO : Patch https://github.com/cpp-netlib/cpp-netlib/pull/460 contains fix for loading
        // certificate chain as string instead of from file. Change client options when this fix is
        // available in cpp-netlib version 0.11.1

        try
        {
            _ksRootCertFilePath = filesystem::temp_directory_path() / filesystem::unique_path();

            filesystem::ofstream certStream(_ksRootCertFilePath);

#ifdef USE_KEYSMITH_STAGING
            certStream.write(reinterpret_cast<const char*>(KEY_SMITH_STAGING_CERTIFICATE),
                             char_traits<unsigned char>::length(KEY_SMITH_STAGING_CERTIFICATE));
#else
            certStream.write(reinterpret_cast<const char*>(KEY_SMITH_PRODUCTION_CERTIFICATE),
                             char_traits<unsigned char>::length(KEY_SMITH_PRODUCTION_CERTIFICATE));
#endif

            certStream.close();
        }
        catch(...)
        {
            throw runtime_error("Initializing KeySmith CA failed.");
        }

        http::client::options options;
        options.openssl_certificate(_ksRootCertFilePath.string());
        _client.reset(new http::client(options));
    }

    static string _GetErrorMessage(http::client::response& response,
                                   const string& propertyName = "message")
    {
        try
        {
            return _ParseJsonProperty(static_cast<string>(body(response)), propertyName);
        }
        catch (const exception& e)
        {
            return static_cast<string>(status_message(response));
        }
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

        if (!_accessToken.empty())
        {
            request << header("Authorization", _GetAuthorizationHeader());
        }

        return _client->delete_(request);
    }

    string _GetAccessToken(const string& refreshToken)
    {
        uri::uri requestUri = _keySmithUrl;
        requestUri << uri::path("/oauth2/authorization/token");

        stringstream requestBody;
        requestBody << "client_id=" << uri::encoded(_clientId.c_str())
                    << "&grant_type=" << uri::encoded("refresh_token")
                    << "&refresh_token=" << uri::encoded(refreshToken.c_str());

        // Access token should be set to empty before calling following _PostRequest,
        // since we are getting new access token
        _accessToken = "";
        http::client::response response =
            _PostRequest(requestUri, requestBody.str(), "application/x-www-form-urlencoded");

        if (status(response) != http_response::ok)
        {
            throw runtime_error(_GetErrorMessage(response).c_str());
        }
        _tokenType = _ParseJsonProperty(body(response), "token_type");

        return _ParseJsonProperty(body(response), "access_token");
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

    void _GetCompanies()
    {
        uri::uri requestUri = _keySmithUrl;
        requestUri << uri::path("/v1/users/me/companies");

        http::client::response response = _GetResponse(requestUri);
        if (status(response) != http_response::ok)
        {
            throw runtime_error(_GetErrorMessage(response).c_str());
        }

        _ParseCompanies(body(response), _companies);

        if (!_companies.size())
        {
            throw runtime_error("User doesn't have any company(s). Please edit your profile"
                                "in KeySmith web page and try again");
        }
    }

    static void _ParseCompanies(const string& json, vector<Company>& companies)
    {
        stringstream jsonStream(json);
        ptree::ptree pt;

        ptree::read_json(jsonStream, pt);

        if (pt.empty())
        {
            throw runtime_error("KeySmith communication failed: Companies response body is empty");
        }

        for (ptree::ptree::value_type& val : pt)
        {
            auto companyProperties = val.second;
            try
            {
                Company company;
                company.Id = companyProperties.get<int>("id");
                company.Name = companyProperties.get<string>("name");
                company.Role = companyProperties.get<string>("role");
                company.JoinedOnInvite =
                    (companyProperties.get<string>("joinedOnInvite") == "true")
                        ? true
                        : false;
                company.CertificateGenerated =
                    (companyProperties.get<string>("certificateGenerated") ==
                     "true")
                        ? true
                        : false;
                company.Certificate = companyProperties.get<string>("certificate");

                companies.emplace_back(company);
            }
            catch(...)
            {
                throw runtime_error("KeySmith communication : Parsing companies failed.");
            }
        }
    }

private:
    filesystem::path _ksRootCertFilePath;

    unique_ptr<http::client> _client;

    uri::uri _keySmithUrl;
    string _clientId;

    uri::uri _pollingEndpoint;

    string _sessionId;
    string _accessToken;
    string _refreshToken;
    string _tokenType;

    vector<Company> _companies;
};

KeySmithClient::KeySmithClient(const string& keySmithUrl, const string& clientId,
                               const std::string& token)
{
    _impl.reset(new Impl(keySmithUrl, clientId, token));
}

KeySmithClient::~KeySmithClient()
{
}

string KeySmithClient::GetLoginUrl()
{
    return _impl->GetLoginUrl();
}

bool KeySmithClient::IsAuthenticated()
{
    return _impl->IsAuthenticated();
}

std::string KeySmithClient::GetToken()
{
    return _impl->GetToken();
}

string KeySmithClient::GetLogoutUrl()
{
    return _impl->GetLogoutUrl();
}

void KeySmithClient::ResetToken()
{
    _impl->ResetToken();
}

void KeySmithClient::GetUserInfo(string& emailId, string& companyName)
{
    _impl->GetUserInfo(emailId, companyName);
}

string KeySmithClient::GetCertificateChain()
{
    return _impl->GetCertificateChain();
}

string KeySmithClient::UploadKdm(const string& kdmXml)
{
    return _impl->UploadKdm(kdmXml);
}

string KeySmithClient::Sign(const string& assetXml)
{
    return _impl->Sign(assetXml);
}

bool KeySmithClient::GetSignedAssetXml(const string& assetId, string& signedXmlAsset)
{
    return _impl->GetSignedAssetXml(assetId, signedXmlAsset);
}
