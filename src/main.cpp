#include "KeySmithClient.h"

#include <memory>
#include <iostream>
#include <chrono>
#include <thread>
#include <fstream>
#include <unistd.h>
#include <cstdlib>

#include <boost/algorithm/string/replace.hpp>

using namespace KEY_SMITH_NS;
using namespace std;

void ShowActionMenu()
{
    cout << endl;
    cout << "1. Sign PKL/CPL." << endl;
    cout << "2. Upload DKDM." << endl;
    cout << "3. Quit." << endl << endl;
    cout << "Please select an action?";
}

void LaunchCommand(string url)
{
    string launchCmd;
#ifdef WIN32
    launchCmd += "start ";
#else
    launchCmd += "open ";
#endif
    launchCmd += url;
    system(launchCmd.c_str());
}

string GetFileContents(const string& filePath)
{
    ifstream fileStream(filePath.c_str());
    if (!fileStream.is_open())
        throw runtime_error("Opening file " + filePath + " for reading failed");

    string content((istreambuf_iterator<char>(fileStream)), (istreambuf_iterator<char>()));
    return content;
}

void WriteToFile(const string& filePath, const string& content)
{
    std::ofstream fileStream(filePath.c_str());
    if (!fileStream.is_open())
        throw runtime_error("Opening file " + filePath + " for writing failed");

    fileStream << content;
}

int main (int argc, char *argv[])
{
    std::unique_ptr<KeySmithClient> keySmithClient;
    try
    {
        if (argc != 2)
            throw runtime_error("Usage: KeySmithClient <Client ID>");

        keySmithClient.reset(new KeySmithClient("https://api.keysmith.com", argv[1]));

        LaunchCommand(keySmithClient->GetLoginUrl());
        cout << "KeySmith sign-in page opened in web browser. Please sign-in to proceed." << endl;

        cout << "Waiting for user to sign-in..." << std::flush;
        while (!keySmithClient->IsAuthenticated())
        {

            this_thread::sleep_for(std::chrono::seconds(2)); // Waiting for 2 seconds to poll again
        }
        cout << endl;

        string email, companyName;
        keySmithClient->GetUserInfo(email, companyName);
        cout << "Successfully signed in as " << email << " (" << companyName << ")" << endl;

        while (true)
        {
            ShowActionMenu();
            int input;
            cin >> input;

            switch (input)
            {
                case 1: // Sign PKL/CPL
                {
                    cout << "Enter unsigned CPL/PKL file path?";
                    string filePath;
                    cin >> filePath;

                    string unsignedXml = GetFileContents(filePath);

                    cout << "Uploading CPL/PKL to KeySmith for signing..." << endl;
                    string xmlId = keySmithClient->Sign(unsignedXml);

                    cout << "Waiting for KeySmith to sign the CPL/PKL..." << std::flush;
                    string signedXml;
                    while (!keySmithClient->GetSignedAssetXml(xmlId, signedXml))
                    {
                        this_thread::sleep_for(std::chrono::seconds(2));
                    }
                    cout << endl;

                    string signedFilePath = boost::ireplace_all_copy(filePath, ".xml", ".signed.xml");
                    WriteToFile(signedFilePath, signedXml);

                    cout << "CPL/PKL successfully signed and available here " << signedFilePath << endl;
                    break;
                }

                case 2: // Upload DKDM
                {
                    cout << "Enter DKDM file path?";
                    string filePath;
                    cin >> filePath;

                    string xml = GetFileContents(filePath);
                    cout << "Uploading DKDM to KeySmith..." << endl;
                    string xmlId = keySmithClient->UploadKdm(xml);

                    // DKDMs are internally signed before getting stored. A successful DKDM sign
                    // indicates DKDM passes all validations and successfully uploaded.
                    cout << "Waiting for KeySmith to compete the DKDM upload..." << std::flush;
                    string statusJson;
                    while (!keySmithClient->GetSignedAssetXml(xmlId, statusJson))
                    {
                        this_thread::sleep_for(std::chrono::seconds(2));
                    }
                    cout << endl;

                    // DKDMs once uploaded can't be retrieved out of KeySmith. But we can generate
                    // DKDM/KDM from it through KeySmith.
                    cout << "DKDM successfully signed and available uploaded DKDM into KeySmith." << endl;
                    break;
                }

                case 3: // Quit
                {
                    // deleting access token ensures that it can't be used again.
                    keySmithClient->ResetToken();
                    return 0;
                }

                default:
                {
                    cout << "Please select a valid option ... " << endl;
                    break;
                }
            }
        }
    }
    catch(const exception& e)
    {
        cout << e.what() << endl;
        try
        {
            // Just to make sure we delete the token in case of failure.
            if (keySmithClient && keySmithClient->GetToken() != "")
                keySmithClient->ResetToken();
        }
        catch (...)
        {
            // intentionally ignored
        }

        return -1;
    }

    return 0;
}
