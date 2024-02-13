#include "parser.hpp"

#include <fstream>
#include <string>
#include <regex>

std::string readFile(const std::string& filePath) {
    std::ifstream file(filePath);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::tuple<OperationRegionInfo,std::string> parseOperationRegion(const std::string& str){
    std::regex re("OperationRegion\\s*\\([0-9A-Za-z\\s]*,[A-Za-z\\s]*,\\s*0[xX]([a-fA-F0-9]{8})\\s*,\\s*0[xX]([a-fA-F0-9]{2})\\s*\\)");
    std::smatch match;
    std::regex_search(str, match, re);
    uint32_t addr = std::stoul(match[1].str(), 0, 16);
    uint32_t size = std::stoul(match[2].str(), 0, 16);
    const auto& opr_info = OperationRegionInfo{addr, size};
    const auto& new_string = str.substr(match.position()+match.length());
    return std::make_tuple(opr_info, new_string);
}

std::vector<Field> extractFields(const std::string& str){
    std::regex re("([A-Za-z0-9_]+|\\s),\\s*([0-9]+)|Offset\\s*\\(0[xX]([0-9A-Fa-f]{2})\\)");
    std::vector<Field> ret;
    std::smatch match;
    std::sregex_iterator iter(str.begin(),str.end(),re);
    std::sregex_iterator end;
    while(iter != end){
        std::smatch match = *iter;
        if (match[3].matched){ // offset
            ret.push_back(Field{FieldType::kOffset, "", static_cast<uint32_t>(std::stoul(match[3].str(), 0, 16))});
        }
        if (match[1].matched){ // named field or padding
            if (match[1].str() == " "){
                ret.push_back(Field{FieldType::kPadding, "", static_cast<uint32_t>(std::stoul(match[2].str()))});
            }
            else {
                ret.push_back(Field{FieldType::kNamedField, match[1].str(), static_cast<uint32_t>(std::stoul(match[2].str()))});
            }
        }
        ++iter;
    }
    return ret;
}