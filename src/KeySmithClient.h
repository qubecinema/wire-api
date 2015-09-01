/**
 * @file KeySmithClient.h
 * @author  Qube Dev <QubeDev@realimage.com>
 *
 * @copyright Copyright &copy; 2015 Qube Cinema Inc. All Rights reserved
 *
 * @brief
 * Contains API(Application Programming Interfaces) to communicate with KeySmith.
 */

#pragma once

#include "NamespaceMacros.h"

#include <vector>
#include <string>
#include <memory>
#include <cstdint>

KEY_SMITH_NS_START

/**
 * KeySmithClient exposes API(Application Programming Interfaces) to communicate with KeySmith.
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
     * Construct KeySmithClient class object.
     *
     * @param[in] keySmithUrl The URL(Uniform Resource Locator) to access KeySmith services
     * @param[in] clientId The unique id of the client application provided by KeySmith
     * @param[in] token The refresh token of a previous KeySmith QAuth session
     */
    KeySmithClient(const std::string& keySmithUrl, const std::string& clientId,
                   const std::string& token = "");

    /**
     * Destruct KeySmithClient class object.
     */
    ~KeySmithClient();

    /**
     * Get KeySmith login URL for this specific client application
     * This method also starts the KeySmith OAuth authentication process by starting a login session
     * that is valid for only 15 minutes.
     * Login is authenticated by polling KeySmithClient::IsAuthenticated till true
     *
     * @returns URL to login to KeySmith
     */
    std::string GetLoginUrl();

    /**
     * Get status of whether user has logged in and access token is available.
     * User should first call KeySmithClient::GetLoginUrl to initiate KeySmith authentication and
     * poll this method till true to complete authentication
     *
     * @returns true if user has logged in and token is available
     */
    bool IsAuthenticated();

    /**
     * @brief Get refresh token for current OAuth session
     *
     * @returns refresh token provided by KeySmith.
     */
    std::string GetToken();

    /**
     * Get URL to terminate current OAuth session and log out of KeySmith
     *
     * @returns URL to log out of KeySmith
     */
    std::string GetLogoutUrl();

    /**
     * Delete the refresh token and access token of the current OAuth session.
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
     * Get certificate chain of active company set by user.
     * The certificate chain contains certificates of leaf, intermediate and root in PEM
     * (Privacy-enhanced Electronic Mail) form concatenated together
     *
     * @returns certificate chain
     */
    std::string GetCertificateChain();

    /**
     * Uploads unsigned KDM for providing Key information to KeySmith
     *
     * @param[in] kdmXml KDM to be uploaded and signed
     *
     * @returns Unique UUID of KDM
     */
    std::string UploadKdm(const std::string& kdmXml);

    /**
     * Posts an asset XML to be signed by KeySmith
     * Asset can be CPL or PKL
     * Use KeySmithClient::IsAssetXmlSigned to know status of the signing process
     *
     * @param[in] assetXml to be signed
     *
     * @returns Unique UUID of CPL or PKL
     */
    std::string Sign(const std::string& assetXml);

    /**
     * Get status of signing of asset, and obtain the signed asset if available
     *
     * @param[in] assetId CPL or PKL UUID
     * @param[out] signedXmlAsset Signed CPL or PKL in string format
     *
     * @returns true if the asset XML is signed and can be retrieved, else false
     */
    bool GetSignedAssetXml(const std::string& assetId, std::string& signedXmlAsset);

private:
    struct Impl;
    std::unique_ptr<Impl> _impl;
};

KEY_SMITH_NS_STOP
