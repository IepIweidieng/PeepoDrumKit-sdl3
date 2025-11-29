#include "../src/core/file_format_fumen.h"
#include <iostream>
#include <cassert>

int main(int argc, char **argv)
{
    if (argc <= 1)
    {
        std::cout << "Usage: " << argv[0] << " <fumen_file_path>\n";
        return 1;
    }
    const char *fumenFilePath = argv[1];
    Fumen::FormatV2::FumenChart chart;
    Fumen::FormatV2::FumenChartReader reader;

    try
    {
        reader.ReadFromFile(fumenFilePath, chart);
        std::cout << "Successfully read fumen file: " << fumenFilePath << std::endl;
    }
    catch (const Fumen::FumenParseException &e)
    {
        std::cerr << "Failed to parse fumen file: " << fumenFilePath << std::endl;
        std::cerr << "Error: " << e.what() << std::endl;
        std::cerr << "Reason: " << e.GetReason() << std::endl;
        std::cerr << "Offset: 0x" << std::hex << e.GetOffset() << " (" << std::dec << e.GetOffset() << " bytes)" << std::endl;
        return 1;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Failed to read fumen file: " << fumenFilePath << std::endl;
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
