#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#include <tuple>


struct OperationRegionInfo{
    uint32_t address;
    uint32_t size;
};

enum class FieldType{
    kPadding,
    kNamedField,
    kOffset  
};

struct Field{
    FieldType type;
    std::string name;
    union {
        uint32_t offset;
        uint32_t bitwidth;
    };
    void print() const{
        if (type == FieldType::kNamedField){
            printf("NamedField: %s %u\n", name.c_str(), bitwidth);
        }
        if (type == FieldType::kOffset){
            printf("Offset: %u\n", offset);
        }
        if (type == FieldType::kPadding){
            printf("Padding: %u\n", bitwidth);
        }
    }
};


std::string readFile(const std::string& filePath);
std::tuple<OperationRegionInfo,std::string> parseOperationRegion(const std::string& str);
std::vector<Field> extractFields(const std::string& str);