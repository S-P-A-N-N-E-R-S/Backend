#include <handling/handlers/abstract_handler.hpp>

namespace server {

graphs::HandlerInformation abstract_handler::createHandlerInformation(const std::string &name,
                                                                      graphs::RequestType type)
{
    graphs::HandlerInformation information;
    information.set_name(name);
    information.set_request_type(type);
    return information;
}

graphs::FieldInformation *abstract_handler::addFieldInformation(
    graphs::HandlerInformation &information, graphs::FieldInformation_FieldType type,
    const std::string &label, const std::string &key, bool required)
{
    auto field = information.add_fields();
    field->set_type(type);
    field->set_label(label);
    field->set_key(key);
    field->set_required(required);
    return field;
}

void abstract_handler::addResultInformation(graphs::HandlerInformation &information,
                                            graphs::ResultInformation_HandlerReturnType type,
                                            const std::string &key, const std::string &label)
{
    auto result = information.add_results();
    result->set_type(type);
    result->set_key(key);
    result->set_label(label);
}

}  // namespace server
