/**
 * @file KeySmithClient.h
 * @author  Qube Dev <QubeDev@realimage.com>
 *
 * @copyright Copyright &copy; 2015 Qube Cinema Inc. All Rights reserved
 *
 * @brief
 * Contains declaration for KeySmithClient class.
 */

#pragma once

#include "NamespaceMacros.h"

#include <vector>
#include <string>
#include <memory>
#include <cstdint>

KEY_SMITH_NS_START

/**
 * KeySmithClient is a wrapper class for KeySmith mastering APIs.
 */
class KeySmithClient
{
public:
    /**
     * Information of company the user belongs to.
     */
    struct Company
    {
        /**
         * Company Id
         */
        int Id;

        /**
         * Company Name
         */
        std::string Name;

        /**
         * Role of user in company
         */
        std::string Role;

        /**
         * Indicates whether the user joined the company on invitaion
         */
        bool JoinedOnInvite;

        /**
         * Indicates whether the company has a certificate
         */
        bool CertificateGenerated;

        /**
         * Certificate representing the company
         */
        std::string Certificate;
    };

    /**
     * Constructs KeySmithClient class object.
     *
     * @param[in] keySmithUrl The URL of the KeySmith
     * @param[in] clientId The unique identifier of the client application provided by KeySmith
     * @param[in] token Optional refresh token from previous KeySmith authorization
     */
    KeySmithClient(const std::string& keySmithUrl, const std::string& clientId,
                   const std::string& token = "");

    /**
     * Destructs KeySmithClient class object.
     */
    ~KeySmithClient();

    /**
     * Get KeySmith login URL.
     * This method also starts the KeySmith authentication process by starting a login session
     * that is valid for only 15 minutes. Authentication status can be obtained by calling
     * KeySmithClient::IsAuthenticated method repeatedly till it returns 'true'.
     *
     * @returns URL to login to KeySmith
     */
    std::string GetLoginUrl();

    /**
     * Get status of whether user has logged in and access token is available.
     * User should first call KeySmithClient::GetLoginUrl to initiate KeySmith authentication and
     * poll this method till true is returned.
     *
     * @returns true if user has logged in and token is available
     */
    bool IsAuthenticated();

    /**
     * Get refresh token for authorization.
     *
     * @returns refresh token provided by KeySmith.
     */
    std::string GetToken();

    /**
     * Get URL for logging out currently logged in KeySmith user.
     *
     * @returns URL to log out of KeySmith
     */
    std::string GetLogoutUrl();

    /**
     * Delete the refresh token and access token of the current authorization.
     * This API should be called after a successful logout to ensure the previous session has ended.
     */
    void ResetToken();

    /**
     * Get user's email id.
     *
     * @param[out] emailId user's email id
     * @param[out] companyName Company name of the user certificate
     */
    void GetUserInfo(std::string& emailId, std::string& companyName);

    /**
     * Get certificate chain of user company.
     * The certificate chain contains certificates of leaf, intermediate and root in PEM form.
     *
     * @returns certificate chain
     */
    std::string GetCertificateChain();

    /**
     * Uploads unsigned DKDM into KeySmith
     * Use KeySmithClient::IsAssetXmlSigned to know status of the upload process.
     *
     * @param[in] kdmXml DKDM XML to be uploaded and signed.
     *
     * @returns Unique identifier of the DKDM
     */
    std::string UploadKdm(const std::string& kdmXml);

    /**
     * Posts an CPL/PKL XML to be signed by KeySmith.
     * Use KeySmithClient::IsAssetXmlSigned to know status of the signing process.
     *
     * @param[in] assetXml CPL/PKL XML to be signed
     *
     * @returns Unique identifier of CPL/PKL
     */
    std::string Sign(const std::string& assetXml);

    /**
     * Get status of asset signing and obtain the signed asset if available.
     *
     * @param[in] assetId CPL/PKL/DKDM unique identifier
     * @param[out] signedXmlAsset Signed CPL/PKL XML. For DKDM this will be empty.
     *
     * @returns true if the asset XML is signed and can be retrieved, else false
     */
    bool GetSignedAssetXml(const std::string& assetId, std::string& signedXmlAsset);

private:
    struct Impl;
    std::unique_ptr<Impl> _impl;
};

KEY_SMITH_NS_STOP
