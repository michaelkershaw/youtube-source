#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <vector>

// Generate a base license key in format YT-XXXX-XXXX-XXXX (3 groups)
// Machine suffix will be appended to make YT-XXXX-XXXX-XXXX-XXXX
std::string GenerateBaseKey() {
    srand((unsigned int)time(nullptr));
    std::ostringstream oss;
    oss << "YT-";
    for (int i = 0; i < 3; i++) {
        if (i > 0) oss << "-";
        for (int j = 0; j < 4; j++) {
            oss << (rand() % 10);
        }
    }
    return oss.str();
}

// Get current timestamp as string
std::string GetTimestamp() {
    auto now = std::time(nullptr);
    auto tm = std::localtime(&now);
    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

// Generate machine-specific key (for demonstration)
std::string GenerateMachineSpecificKey(const std::string& baseKey, const std::string& machineID) {
    std::string machineSuffix = machineID.length() >= 4 
        ? machineID.substr(machineID.length() - 4)
        : "0000";
    return baseKey + "-" + machineSuffix;
}

int main() {
    std::cout << "=== YouTube Source License Key Generator ===" << std::endl;
    std::cout << std::endl;

    // Generate a new base key
    std::string baseKey = GenerateBaseKey();
    std::cout << "Generated Base License Key: " << baseKey << std::endl;
    std::cout << std::endl;

    // Ask for customer info
    std::string customerName, email;
    std::cout << "Enter customer name: ";
    std::getline(std::cin, customerName);
    std::cout << "Enter customer email: ";
    std::getline(std::cin, email);

    if (customerName.empty() || email.empty()) {
        std::cout << "Error: Customer name and email are required." << std::endl;
        return 1;
    }

    // REQUIRE: Customer must provide their Machine ID for key binding
    std::string machineID;
    std::string finalKey;
    
    std::cout << std::endl;
    std::cout << "=== MACHINE BINDING REQUIRED ===" << std::endl;
    std::cout << "This license system requires binding to a specific machine." << std::endl;
    std::cout << std::endl;
    std::cout << "To get the Machine ID:" << std::endl;
    std::cout << "1. Install the YouTube Source plugin on the target machine" << std::endl;
    std::cout << "2. Open VirtualDJ and check the logs" << std::endl;
    std::cout << "3. Look for: 'Machine ID: XXXXXXXX'" << std::endl;
    std::cout << std::endl;
    std::cout << "Enter the customer's Machine ID (8-character hex): ";
    std::getline(std::cin, machineID);
    
    // Validate machine ID format
    while (machineID.length() != 8 || machineID.find_first_not_of("0123456789ABCDEFabcdef") != std::string::npos) {
        std::cout << "Invalid Machine ID! Must be exactly 8 hexadecimal characters (0-9, A-F)." << std::endl;
        std::cout << "Enter the customer's Machine ID: ";
        std::getline(std::cin, machineID);
    }
    
    // Convert to uppercase for consistency
    for (char &c : machineID) {
        c = toupper(c);
    }
    
    // Generate machine-specific key
    finalKey = GenerateMachineSpecificKey(baseKey, machineID);
    std::cout << std::endl;
    std::cout << "Generated Machine-Bound Key: " << finalKey << std::endl;
    std::cout << "This key will ONLY work on machine with ID: " << machineID << std::endl;

    // Log to registrations file
    std::ofstream logFile("registrations.log", std::ios::app);
    if (logFile.is_open()) {
        logFile << "[" << GetTimestamp() << "] BASE_KEY: " << baseKey 
                << " | FINAL_KEY: " << finalKey
                << " | CUSTOMER: " << customerName 
                << " | EMAIL: " << email;
        if (!machineID.empty()) {
            logFile << " | MACHINE: " << machineID;
        }
        logFile << std::endl;
        logFile.close();
        std::cout << "Registration logged to: registrations.log" << std::endl;
    } else {
        std::cout << "Warning: Could not open registrations.log for writing." << std::endl;
    }

    // Save to keys database (simple CSV)
    std::ofstream keysFile("keys.csv", std::ios::app);
    if (keysFile.is_open()) {
        keysFile << baseKey << "," << finalKey << "," << customerName << "," << email << "," << GetTimestamp();
        if (!machineID.empty()) {
            keysFile << "," << machineID;
        }
        keysFile << std::endl;
        keysFile.close();
        std::cout << "Key saved to: keys.csv" << std::endl;
    } else {
        std::cout << "Warning: Could not open keys.csv for writing." << std::endl;
    }

    std::cout << std::endl;
    std::cout << "=== LICENSE INFORMATION ===" << std::endl;
    std::cout << "Base Key: " << baseKey << std::endl;
    std::cout << "Final Key: " << finalKey << std::endl;
    std::cout << "Customer: " << customerName << std::endl;
    std::cout << "Email: " << email << std::endl;
    if (!machineID.empty()) {
        std::cout << "Bound to Machine: " << machineID << std::endl;
    }
    std::cout << std::endl;
    std::cout << "Send this key to the customer: " << finalKey << std::endl;
    std::cout << "The key will be valid for 1 year from activation." << std::endl;
    std::cout << std::endl;
    std::cout << "IMPORTANT: The key is bound to the machine's hardware fingerprint." << std::endl;
    std::cout << "It cannot be transferred to another computer." << std::endl;

    return 0;
}
