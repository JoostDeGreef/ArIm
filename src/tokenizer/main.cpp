#include <cstdio>
#include <charconv>
#include <format>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <string>
#include <system_error>
#include <unistd.h>

#include "tokenizer.h"
#include "db.h"
#include "utils.h"

int main(int argc, char ** argv)
{
    std::string dbFilename;
    Tokenizer t;
    std::vector<std::string> inputFiles;
    size_t activeTokenCountMax = 1024;
    size_t stepSize = 1;
    bool showStatistics = false;

    auto usage = [argv](std::ostream & out) 
    {
        out << "Usage: " << argv[0]
            << " -o OUTPUT.sq3 [-m MAX_TOKENS] [-s STEP] INPUT...\n"
            << "  -o FILE  SQLite output filename (required)\n"
            << "  -m NUM   maximum active token count (default: 1024)\n"
            << "  -s NUM   optimization step size (default: 1)\n"
            << "  -S       show statistics after running\n"
            << "  -h       show this help\n";
    };
    auto parseSize = [](const char * text, size_t & value) 
    {
        if (!text || *text == '\0' || *text == '-') 
        {
          return false;
        }
        unsigned long long parsed = 0;
        const char * end = text + std::char_traits<char>::length(text);
        auto result = std::from_chars(text, end, parsed);
        if (result.ec != std::errc{} || 
            result.ptr != end ||
            parsed > std::numeric_limits<size_t>::max())
        {
          return false;
        } 
        value = static_cast<size_t>(parsed);
        return true;
    };

    int option;
    while ((option = getopt(argc, argv, "o:m:s:hS")) != -1) 
    {
        switch (option) 
        {
        case 'o': 
            dbFilename = optarg; 
            break;
        case 'm':
            if (!parseSize(optarg, activeTokenCountMax) || activeTokenCountMax == 0) 
            {
                std::cerr << "Invalid maximum token count: " << optarg << '\n';
                usage(std::cerr);
                return EXIT_FAILURE;
            }
            break;
        case 's':
            if (!parseSize(optarg, stepSize) || stepSize == 0) {
                std::cerr << "Invalid step size: " << optarg << '\n';
                usage(std::cerr);
                return EXIT_FAILURE;
            }
            break;
        case 'S':
            showStatistics = true;
            break;
        case 'h':
            usage(std::cout); 
            return EXIT_SUCCESS;
        default: 
            usage(std::cerr); 
            return EXIT_FAILURE;
        }
    }
    if (dbFilename.empty() || optind == argc) 
    {
        usage(std::cerr);
        return EXIT_FAILURE;
    }
    for (int i = optind; i < argc; ++i) inputFiles.emplace_back(argv[i]);

    auto readData = [](const std::string & filename) 
    {
        std::ifstream file(filename, std::ios::binary);
        if (!file) 
        {
          throw std::runtime_error("Cannot open input file: " + filename);
        }
        return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    };

    SQLite::DB db;
    db.Open(dbFilename, false);
    db.ExecDML("DROP TABLE IF EXISTS Statistics");
    db.ExecDML("CREATE TABLE Statistics(\"Group\" TEXT, Key TEXT, Value TEXT, Description TEXT);");

    auto writeStatistic = [&db](const std::string & group, const std::string & key, const std::string & value, const std::string & description = "")
    {
      db.ExecDML(std::format("INSERT INTO Statistics(\"Group\", Key, Value, Description) VALUES('{}','{}','{}','{}')",group,key,value,description));
    };

    writeStatistic("settings","activeTokenCountMax",std::to_string(activeTokenCountMax));
    writeStatistic("settings","stepSize",std::to_string(stepSize));

    {
      size_t totalBytesInput = 0;
      Timer readInputFileTimer([&](std::int64_t duration)
      {
        writeStatistic("total input", "bytes", std::to_string(totalBytesInput));
        writeStatistic("total input", "duration (ms)", std::to_string(duration));
      });
      try 
      {
        for (size_t i = 0; i<inputFiles.size();++i) 
        {
          const auto & filename = inputFiles[i];
          size_t inputSize = 0;
          Timer readInputFileTimer([&](std::int64_t duration)
          {
            writeStatistic("input file " + std::to_string(i), "filename", filename);
            writeStatistic("input file " + std::to_string(i), "size", std::to_string(inputSize));
            writeStatistic("input file " + std::to_string(i), "duration (ms)", std::to_string(duration));
          });
          std::cout << "Tokenizing " << filename << std::endl;
          const std::string fileData = readData(filename);
          inputSize = fileData.size();
          t.FeedInput(fileData);
          totalBytesInput += inputSize;
        }
      } 
      catch (const std::exception & error) 
      {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
      }
    }

    {
      Timer readInputFileTimer([&](std::int64_t duration)
      {
        writeStatistic("OptimizeTokenLength", "duration (ms)", std::to_string(duration));
      });
      t.OptimizeTokenLength(activeTokenCountMax,stepSize);
    }

    {
      Timer readInputFileTimer([&](std::int64_t duration)
      {
        writeStatistic("CalculateTokenRelationships", "duration (ms)", std::to_string(duration));
      });
      t.CalculateTokenRelationships();
    }

    {
      Timer readInputFileTimer([&](std::int64_t duration)
      {
        writeStatistic("StoreCalculatedDataToDB", "duration (ms)", std::to_string(duration));
      });
      t.StoreCalculatedDataToDB(dbFilename);
    }

    if(showStatistics)
    {
      auto query = db.ExecQuery("SELECT \"Group\", Key, Value FROM Statistics ORDER BY rowid");
      std::string currentInputGroup;
      bool settingsCaptionShown = false;
      bool inputCaptionShown = false;
      while (!query.IsEOF())
      {
        const auto group = query.GetStringField("Group");
        const auto key = query.GetStringField("Key");
        const auto value = query.GetStringField("Value");
        if (group.rfind("input file ", 0) == 0)
        {
          if (!inputCaptionShown) { std::cout << "Input:" << '\n'; inputCaptionShown = true; }
          if (group != currentInputGroup)
          {
            if (!currentInputGroup.empty()) std::cout << '\n';
            currentInputGroup = group;
            std::cout << "  ";
          }
          else std::cout << " | ";
          std::cout << key << ": " << value;
        }
        else if (group == "total input")
        {
          if (!currentInputGroup.empty()) { std::cout << '\n'; currentInputGroup.clear(); }
          std::cout << "  Total " << key << ": " << value << '\n';
        }
        else if (group == "settings")
        {
          if (!currentInputGroup.empty()) { std::cout << '\n'; currentInputGroup.clear(); }
          if (!settingsCaptionShown) { std::cout << "Settings:" << '\n'; settingsCaptionShown = true; }
          std::cout << "  " << key << ": " << value << '\n';
        }
        else
        {
          if (!currentInputGroup.empty()) { std::cout << '\n'; currentInputGroup.clear(); }
          std::cout << group << ": " << key << ": " << value << '\n';
        }
        query.NextRow();
      }
      if (!currentInputGroup.empty()) std::cout << '\n';
      query.Finalize();
    }

    return EXIT_SUCCESS;
}
 
