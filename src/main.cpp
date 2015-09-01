#include "KeySmithClient.h"

#include <memory>
#include <iostream>
#include <chrono>
#include <thread>
#include <fstream>
#include <unistd.h>
#include <cstdlib>

using namespace KEY_SMITH_NS;
using namespace std;

void ShowOptions()
{
    cout << endl << "1. Sign PKL/CPL." << endl;
    cout << "2. Upload DKDM." << endl;
    cout << "3. Quit." << endl << endl;
    cout << "Select an option: ";
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

string GetFileContents(string filePath)
{
    ifstream fileStream(filePath.c_str());
    string content((istreambuf_iterator<char>(fileStream)), (istreambuf_iterator<char>()));
    return content;
}

void WriteToFile(string filePath, string content)
{
    std::ofstream fileStream(filePath.c_str());
    fileStream << content;
}

int main (int argc, char *argv[])
{
    std::unique_ptr<KeySmithClient> keySmithClient;
    try
    {
        if (argc != 2)
            throw runtime_error("Usage: KeySmithClient <ClientId>");
        
        keySmithClient.reset(new KeySmithClient("https://api.keysmith.com", argv[1]));
        
        cout << "Launching KeySmith sign in page in default browser - Please wait ..." << endl;
        LaunchCommand(keySmithClient->GetLoginUrl());
        
        while (!keySmithClient->IsAuthenticated())
        {
            // TODO: "..." should be animated
            cout << '\r' << "Please sign in to KeySmith to proceed ..." << std::flush;

            this_thread::sleep_for(std::chrono::seconds(2)); // Waiting for 2 seconds to poll again
        }
        cout << endl << "Fetching user information - Please wait ..." << endl;
        string email, companyName;
        keySmithClient->GetUserInfo(email, companyName);
        cout << "Successfully signed in as " << email.c_str() << " (" << companyName.c_str() << ")" << endl;

        while (true)
        {
            ShowOptions();
            int input;
            cin >> input;
            
            switch (input)
            {
                case 1:
                {
                    // Sign PKL/CPL
                    cout << "Enter the path of unsigned CPL/PKL XML file: ";
                    string cplOrPklPath;
                    cin >> cplOrPklPath;
                    
                    string cplOrPkl = GetFileContents(cplOrPklPath);
                    cout << "Sending CPL/PKL for signing ..." << endl;
                    string cplOrPklId = keySmithClient->Sign(cplOrPkl);
                    
                    string signedCplOrPklXml;
                    while (!keySmithClient->GetSignedAssetXml(cplOrPklId, signedCplOrPklXml))
                    {
                        // TODO: "..." should be animated
                        cout << '\r' << "Please wait while we sign CPL/PKL xml ..." << std::flush;

                        this_thread::sleep_for(std::chrono::seconds(2)); // waiting for 2 seconds to poll again
                    }
                    string outputFile = cplOrPklPath;
                    outputFile += ".Signed.xml";
                    WriteToFile(outputFile, signedCplOrPklXml);
                    cout << endl << "Successfully signed CPL/PKL and saved into " << outputFile.c_str() << endl;
                    break;
                }
                case 2:
                {
                    // Upload DKDM
                    cout << "Enter the path of DKDM XML file: ";
                    string kdmPath;
                    cin >> kdmPath;
                    string kdm = GetFileContents(kdmPath);
                    cout << "Sending DKDM to KeySmith ..." << endl;
                    string kdmId = keySmithClient->UploadKdm(kdm);
                    string kdmXml;
                    while (!keySmithClient->GetSignedAssetXml(kdmId, kdmXml))
                    {
                        // TODO: "..." should be animated
                        cout << '\r' << "Please wait while we sign DKDM xml ..." << std::flush;

                        this_thread::sleep_for(std::chrono::seconds(2)); // waiting for 2 seconds to poll again
                    }
                    cout << endl << "Successfully uploaded DKDM into KeySmith." << endl;
                    break;
                }
                case 3:
                {
                    // Delete Token and Quit
                    cout << "Signing out from KeySmith - Please wait ..." << endl;
                    keySmithClient->ResetToken();
                    LaunchCommand(keySmithClient->GetLogoutUrl());
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
            if (keySmithClient.get() && keySmithClient->GetToken() != "")
                keySmithClient->ResetToken();
        }
        catch (...)
        {
            // Purposefully left blank
        }

        return -1;
    }
    catch(...)
    {
        cout << "Unexpected exception occured" << endl;
        try
        {
            // Just to make sure we delete the token in case of failure.
            if (keySmithClient.get())
                keySmithClient->ResetToken();
        }
        catch (...)
        {
            // Purposefully left blank
        }

        return -2;
    }

    return 0;
}
