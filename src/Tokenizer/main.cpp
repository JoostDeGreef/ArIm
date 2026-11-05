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

#include "Tokenizer.h"
#include "SQLiteDB.h"
#include "Utils.h"

int main(int argc, char ** argv)
{
    std::string dbFilename;
    Tokenizer t;
    std::vector<std::string> inputFiles;
    bool showStatistics = false;

    auto usage = [argv](std::ostream & out) 
    {
        out << "Usage: " << argv[0]
            << " -o OUTPUT.sq3 [-m MAX_TOKENS] INPUT...\n"
            << "  -o FILE  SQLite output filename (required)\n"
            << "  -s       show statistics after running\n"
            << "  -h       show this help\n";
    };

    int option;
    while ((option = getopt(argc, argv, "o:sh")) != -1) 
    {
        switch (option) 
        {
        case 'o': 
            dbFilename = optarg; 
            break;
        case 's':
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

    db.ExecDML("PRAGMA journal_mode = OFF;");
    db.ExecDML("PRAGMA synchronous = OFF;");
    db.ExecDML("PRAGMA temp_store = MEMORY;");

    db.ExecDML("DROP TABLE IF EXISTS Statistics");
    db.ExecDML("CREATE TABLE Statistics(\"Group\" TEXT, Key TEXT, Value TEXT, Description TEXT);");

    auto writeStatistic = [&db](const std::string & group, const std::string & key, const std::string & value, const std::string & description = "")
    {
      db.ExecDML(std::format("INSERT INTO Statistics(\"Group\", Key, Value, Description) VALUES('{}','{}','{}','{}')",group,key,value,description));
    };

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
          t.Process(fileData);
          totalBytesInput += inputSize;
        }
      } 
      catch (const std::exception & error) 
      {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
      }
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

    db.ExecDML("PRAGMA analysis_limit=400;");
    db.ExecDML("PRAGMA optimize;");
    db.Close();
    
    return EXIT_SUCCESS;
}
 
