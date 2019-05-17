#include <utility>


#pragma once

#include <fs/core/fstrips/fol.hxx>
#include <fs/core/fstrips/language_info.hxx>
#include <fs/core/fstrips/language.hxx>
#include <fs/core/fstrips/interpretation.hxx>


namespace fs0::fstrips {

class Grounding {
public:
    Grounding(std::shared_ptr<LanguageInfo> language) : language_(std::move(language)) {}

    VariableIdx add_state_variable(const symbol_id& symbol, const std::vector<object_id>& point);

    std::string compute_state_variable_name(const symbol_id& symbol, const std::vector<object_id>& point);

protected:
    std::shared_ptr<LanguageInfo> language_;

    /// Ground state variables ///
    //! A map from state variable ID to state variable name
    std::vector<std::string> variableNames;

    //! A map from state variable name to state variable ID
    std::unordered_map<std::string, VariableIdx> variableIds;

    //! A map from the actual data "f(t1, t2, ..., tn)" to the assigned variable ID
    std::map<std::pair<unsigned, std::vector<object_id>>, VariableIdx> variableDataToId;
    std::vector<std::pair<unsigned, std::vector<object_id>>> variableIdToData;

    //! Mapping from state variable index to the type associated to the state variable
    std::vector<type_id> _sv_types;

    //! Maps variable index to type index
    std::vector<TypeIdx> variableTypes;

    /// Ground actions ///
};

} // namespaces
