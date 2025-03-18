#pragma once
#include "PluginRunnableInterface.hpp"

class Email;

// EmailListFilter class implementing PluginInterface
class EmailListFilter final : public PluginRunnableInterface {
public:
    EmailListFilter(const std::string&);  // Constructor
    ~EmailListFilter() override;  // Destructor

    bool instantiateRecursive() override;
    nlohmann::json printRecursiveInstanceTreeJson() override;

    bool execute(EmailListView *emailList) override;
private:
    //bool matchesFilter(const std::string& value, const nlohmann::json& filter) const;
    std::pmr::map<std::string, std::vector<std::string>> filters;
    struct filterStruct {
        std::string field;
        std::string outcome;
        std::string filterBy;
        std::vector<std::string> values;
    };
    void processEmail(const Email& email, EmailListView* emailList, const filterStruct& emailFilter) const;

    struct Register {
        Register() {
            std::string name = "EmailListFilter";
            LOG_DEBUG_VERBOSE << "Registering plugin " << name;
            Plugins->registerPlugin(name, [](const std::string& instanceID) -> PluginInterface* {
                return new EmailListFilter(instanceID);
            });
        }
    };
    static inline Register reg;
};
