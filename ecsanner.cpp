#include <argparse/argparse.hpp>
#include <fcntl.h>
#include <string.h>
#include <string>
#include <sys/mman.h>
#include <tuple>
#include "parser.hpp"
#include <format>


class NotImplemented : public std::logic_error
{
public:
    NotImplemented() : std::logic_error("Function not yet implemented") { };
};


struct NamedField{
    std::string name;
    uint32_t bitmask;
    uint32_t byte_offset;  
    uint32_t bytes;
    void print() const{
        printf("[%s] Dword Mask: 0x%08x, Byte Offset: 0x%02x, Size: %02u\n", name.c_str(), bitmask, byte_offset, bytes);
    }
};

struct ECRamTable{
    uint32_t address;
    uint32_t size;
    std::vector<NamedField> fields;  
    std::unordered_map<std::string, size_t> field_map;

    static ECRamTable buildECRamTable(const OperationRegionInfo& opr_info, const std::vector<Field>& field_vec){
        ECRamTable ret;
        ret.address = opr_info.address;
        ret.size = opr_info.size;
        uint32_t bit_offset = 0;
        for (const auto& field : field_vec){
            switch (field.type){
                case FieldType::kNamedField:{
                    uint32_t byte_offset = bit_offset / 8;
                    uint32_t new_bit_offset = bit_offset + field.bitwidth;
                    uint32_t _bit = byte_offset * 8;
                    uint32_t _lo = bit_offset - _bit;
                    uint32_t _hi = new_bit_offset - _bit;
                    /*
                        xxxxXXXXXXXXXXXXxxxxxxxxxxx
                        |   |           |
                        |   bit_offset  new_bit_offset
                        |
                        _bit
                    */
                    decltype(NamedField::bitmask) bitmask = (~(std::numeric_limits<int64_t>::max() << _hi)) & (std::numeric_limits<int64_t>::max() << _lo);
                    ret.fields.emplace_back(NamedField{field.name, bitmask, byte_offset, (_hi+7)/8});
                    ret.field_map[field.name] = ret.fields.size() - 1;
                    bit_offset += field.bitwidth;
                    break;
                }
                case FieldType::kPadding:{
                    bit_offset += field.bitwidth;
                    break;
                }
                case FieldType::kOffset:{
                    bit_offset = field.offset * 8;
                    break;
                }
            }
        }
        return ret;
    }

    std::optional<NamedField> getField(const std::string& name) {
        if (field_map.find(name) == field_map.end()){
            return std::nullopt;
        } else {
            return fields.at(field_map.at(name));
        }
    }
    
    std::optional<NamedField> getField(const std::string& name) const {
        if (field_map.find(name) == field_map.end()){
            return std::nullopt;
        } else {
            return fields.at(field_map.at(name));
        }
    }
};


constexpr size_t PAGE_SIZE = 4096;
constexpr size_t PAGE_MASK = (PAGE_SIZE - 1);

class MappedMemory{
public:
    MappedMemory(void* target, size_t size): target(target){
        int fd;
        fd = open("/dev/mem", O_RDWR|O_SYNC);
        if (fd == -1){
            throw std::runtime_error(std::format("Failed to open /dev/mem : {}", strerror(errno)));
        }
        map_base = mmap(target, size, PROT_READ, MAP_SHARED, fd , reinterpret_cast<size_t>(target) & ~PAGE_MASK);
        map_size = size;
    }
    ~MappedMemory(){
        munmap(map_base, map_size);
    }

    void* getVirt() const{
        size_t map_base_sizet = reinterpret_cast<size_t>(map_base) + (reinterpret_cast<size_t>(target) & PAGE_MASK);
        return reinterpret_cast<void*>(map_base_sizet);
    }

    void* map_base;
    void* target;
    size_t map_size;
};

inline uint32_t readNamedField(const NamedField& field, const MappedMemory& mapped_memory){
    uint32_t value = 0;
    memcpy(&value, static_cast<uint8_t*>(mapped_memory.getVirt()) + field.byte_offset, field.bytes);
    value = value & field.bitmask;
    int bit_index = __builtin_ctz(field.bitmask);
    value = value >> bit_index;
    return value;
}

std::vector<std::tuple<std::string, uint32_t>> readECRamTable(const ECRamTable& ec_ram_table, const MappedMemory& mapped_memory){
    std::vector<std::tuple<std::string, uint32_t>> ret;
    for (const auto& field : ec_ram_table.fields){
        auto value = readNamedField(field, mapped_memory);
        ret.emplace_back(std::make_tuple(field.name, value));
    }
    return ret;
}

std::optional<std::tuple<std::string, uint32_t>> queryECRamTable(const ECRamTable& ec_ram_table, const MappedMemory& mapped_memory, const std::string& field_name){
    const auto& field = ec_ram_table.getField(field_name);
    if (field){
        auto value = readNamedField(*field, mapped_memory);
        return std::make_tuple(field->name, value);
    } else {
        printf("Field %s not found, ignoring\n", field_name.c_str());
        return std::nullopt;
    }
}

int show(const std::string& filename){
    auto file_str = readFile(filename);
    const auto& [opr_info, newstr] = parseOperationRegion(file_str);
    const auto& field_vec = extractFields(newstr);
    const auto& ec_ram_table = ECRamTable::buildECRamTable(opr_info, field_vec);
    printf("ECRam Mapped address: 0x%08X Size: %u\n", ec_ram_table.address, ec_ram_table.size);
    for (const auto& field : ec_ram_table.fields){
        field.print();
    }
    return 0;
}

int scan(const std::string& filename){
    auto file_str = readFile(filename);
    const auto& [opr_info, newstr] = parseOperationRegion(file_str);
    const auto& field_vec = extractFields(newstr);
    const auto& ec_ram_table = ECRamTable::buildECRamTable(opr_info, field_vec);
    MappedMemory mapped_memory(reinterpret_cast<void*>(ec_ram_table.address), ec_ram_table.size);
    const auto& values = readECRamTable(ec_ram_table, mapped_memory);
    for (const auto& [name, value] : values){
        printf("[%s] 0x%08X\n", name.c_str(), value);
    }
    return 0;
}

int query(const std::string& filename, const std::vector<std::string>& field_names){
    auto file_str = readFile(filename);
    const auto& [opr_info, newstr] = parseOperationRegion(file_str);
    const auto& field_vec = extractFields(newstr);
    const auto& ec_ram_table = ECRamTable::buildECRamTable(opr_info, field_vec);
    MappedMemory mapped_memory(reinterpret_cast<void*>(ec_ram_table.address), ec_ram_table.size);
    for (const auto& field_name : field_names){
        const auto& query_result = queryECRamTable(ec_ram_table, mapped_memory, field_name);
        if (query_result){
            const auto& [name, value] = *query_result;
            printf("[%s] 0x%08X\n", name.c_str(), value);
        }
    }
    return 0;
}



int main(int argc, char *argv[]) {
    argparse::ArgumentParser program("ecscanner");
    program.add_description("A tool for reading EC RAM");
    program.add_argument("action")
        .help("action to take: [show] ECRam symbol table, [scan] ECRam symbol values, [query] ECRam symbol, [monitor] ECRam symbol values")
        .choices("show", "scan", "query", "monitor");
    program.add_argument("filename")
        .help("input file")
        .metavar("FILENAME");
    program.add_argument("field")
        .help("field names for query mode")
        .nargs(argparse::nargs_pattern::any);
    

    try {
      program.parse_args(argc, argv);
    }
    catch (const std::exception& err) {
      std::cerr << err.what() << std::endl;
      std::cerr << program;
      return 1;
    }

    std::string filename = program.get<std::string>("filename");
    std::string action = program.get<std::string>("action");
    std::vector<std::string> queries = program.get<std::vector<std::string>>("field");


    if (action == "show"){
        show(filename);
    }
    else if (action == "scan"){
        scan(filename);
    }
    else if (action == "query"){
        query(filename, queries);
    }
    else if (action == "monitor"){
        throw NotImplemented();
    }

    return 0;
}
