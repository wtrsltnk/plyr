#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#include <Base64.h>

int main(
    int argc,
    char *argv[])
{
    if (argc < 2)
    {
        std::cout << "missing image path" << std::endl;
        return 1;
    }

    for (int i = 1; i < argc; i++)
    {
        std::ifstream file(argv[i], std::ios::binary | std::ios::ate);
        std::vector<unsigned char> buffer(file.tellg());

        file.seekg(0);
        file.read(reinterpret_cast<char *>(buffer.data()), buffer.size());

        std::cout << "inline constexpr char " << std::filesystem::path(argv[i]).stem().string() << "Image = \"" << macaron::Base64::Encode(buffer) << "\";" << std::endl;
    }

    return 0;
}
