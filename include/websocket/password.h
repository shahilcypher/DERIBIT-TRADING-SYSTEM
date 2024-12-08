#pragma once
#include <string>

class Password {
private:
    static int num_sets;
    std::string access_token;

    // Private constructor for singleton
    Password() : access_token("") {}

public:
    // Singleton accessor
    static Password& password();

    // Delete copy constructor and assignment operator
    Password(const Password&) = delete;
    void operator=(const Password&) = delete;

    // Methods to set and get access token
    void setAccessToken(const std::string& token);
    void setAccessToken(int& token);
    std::string getAccessToken() const;
};