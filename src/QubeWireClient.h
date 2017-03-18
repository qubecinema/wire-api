/**
 * @file QubeWireClient.h
 *
 * @copyright Copyright &copy; 2017 Qube Cinema Inc. All Rights reserved
 *
 * @brief
 * Contains API(Application Programming Interfaces) to communicate with Qube Wire.
 */

#pragma once

#include "NamespaceMacros.h"

#include <vector>
#include <string>
#include <memory>
#include <cstdint>

QUBE_WIRE_NS_START

/**
 * QubeWireClient exposes API(Application Programming Interfaces) to communicate with Qube Wire.
 */
class QubeWireClient
{
public:
    /**
     * Construct QubeWireClient class object.
     *
     * @param[in] clientId Unique client identifier to communicate with Qube Wire
     */
    QubeWireClient(const std::string& clientId);

    /**
     * Destruct QubeWireClient class object.
     */
    ~QubeWireClient();

    /**
     * Get Qube Wire login URL for this specific client application
     * This method also starts the Qube Wire OAuth authentication process by starting a login session
     * that is valid for only 15 minutes.
     * Login is authenticated by polling QubeWireClient::IsAuthenticated till true
     *
     * @returns URL to login to Qube Wire
     */
    std::string GetLoginUrl();

    /**
     * Get status of whether user has logged in and access token is available.
     * User should first call QubeWireClient::GetLoginUrl to initiate Qube Wire authentication and
     * poll this method till true to complete authentication
     *
     * @returns true if user has logged in and token is available
     */
    bool IsAuthenticated();

    /**
     * @brief Get refresh token for current OAuth session
     *
     * @returns refresh token provided by Qube Wire.
     */
    std::string GetToken();

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
     * form concatenated together
     *
     * @returns certificate chain
     */
    std::string GetCertificateChain();

    /**
     * Uploads unsigned KDM for providing Key information to Qube Wire
     *
     * @param[in] kdmXml KDM to be uploaded and signed
     *
     * @returns unique identifier of the KDM
     */
    std::string UploadKdm(const std::string& kdmXml);

    /**
     * Posts an asset XML to be signed by Qube Wire
     * Asset can be CPL or PKL
     * Use QubeWireClient::IsAssetXmlSigned to know status of the signing process
     *
     * @param[in] assetXml to be signed
     *
     * @returns unique identifier of the asset
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

QUBE_WIRE_NS_STOP
