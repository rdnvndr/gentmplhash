/******************************************************************************
*  Программа создает XML файл с контрольными суммами MD5 для файлов с
*  расширением *. vrp, которые находятся в каталоге, указанном в первом
*  аргументе программы, и отравляет его в std:cout. Если std::cin содержит
*  ранее сгенерированный поток, то он обновляет при помощи добавления для
*  файла ранее не существовавшей контрольной суммы MD5
******************************************************************************/
#include <iostream>
#include <string>
#include <cwchar>

#include <locale>
#include <codecvt>
#include <cwctype>
#include <algorithm>
#include <vector>

#include <sys/types.h>
#include <sys/stat.h>

#include "tinydir/tinydir.h"
#include "md5/md5.h"
#include "tinyxml2/tinyxml2.h"

using namespace tinyxml2;

//! Конвертирует wstring в mbstring
std::string wdtomb(const wchar_t* wstr)
{
    std::mbstate_t state = std::mbstate_t();
    std::size_t len = 1 + std::wcsrtombs(nullptr, &wstr, 0, &state);
    std::vector<char> mbstr(len);
    std::wcsrtombs(&mbstr[0], &wstr, mbstr.size(), &state);

   return std::string(&mbstr[0]);
}

//! Конвертирует mbstring в wstring
std::wstring mbtowd(const char* mbstr)
{
    std::mbstate_t state = std::mbstate_t();
    std::size_t len = 1 + std::mbsrtowcs(nullptr, &mbstr, 0, &state);
    std::vector<wchar_t> wstr(len);
    std::mbsrtowcs(&wstr[0], &mbstr, wstr.size(), &state);

    return std::wstring(&wstr[0]);
}

//! Конвертирует Utf16 в Utf8
std::string toUtf8(const std::wstring &wstr) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;

    return converter.to_bytes(wstr);
}

//! Конвертирует Utf8 в Utf16
std::wstring toUtf16(const std::string &str) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;

    return converter.from_bytes(str);
}

int main(int argc, char *argv[])
{
    setlocale(LC_ALL,"");

    struct stat info;
    if (argc < 2 || stat(argv[1], &info) != 0 || !(info.st_mode & S_IFDIR))
        return 0;

    XMLDocument doc;
    std::string str(std::istreambuf_iterator<char>(std::cin), {});
    if (str.empty()) {
        doc.InsertFirstChild(doc.NewDeclaration());
    } else if (doc.Parse(str.c_str()) != XMLError::XML_SUCCESS) {
        std::wcout << L"Неверный формат входных данных" << std::endl;
        return 0;
    }

    auto hashtable = doc.FirstChildElement("hashtable");
    if (hashtable == nullptr) {
        hashtable = doc.NewElement("hashtable");
        doc.InsertEndChild(hashtable);
    }

    tinydir_file file;
    tinydir_dir dir;
    tinydir_open(&dir, argv[1]);
    while (dir.has_next)
    {
        tinydir_readfile(&dir, &file);
        auto wdFileName = mbtowd(file.name);
        size_t len = wdFileName.length();
        if (!file.is_dir && len > 4 && wdFileName.compare(len - 4, 4, L".vrp") == 0)
        {
            // Получение имя файла в нижнем регистре без расширения
            wdFileName.erase(wdFileName.end() - 4, wdFileName.end());
            std::transform(wdFileName.begin(), wdFileName.end(),
                           wdFileName.begin(),
                           [](const wchar_t  &c){ return std::towlower(c); }
            );
            auto lowerFileName = toUtf8(wdFileName);

            // Поиск описания файла с его хэшами
            auto item = hashtable->FirstChildElement("item");
            while (item != nullptr && lowerFileName != item->Attribute("key")) {
                item = item->NextSiblingElement("item");
            }

            auto tmplHash = md5file(std::string(argv[1] + wdtomb(L"/") + file.name).c_str());
            XMLElement *value = nullptr;
            if (item == nullptr) {
                // Добавляет описание файла
                item = doc.NewElement("item");
                hashtable->InsertEndChild(item);
                item->SetAttribute("key", lowerFileName.c_str());
            } else {
                // Поиск значения хэша
                value = item->FirstChildElement("value");
                while (value != nullptr && tmplHash != value->GetText()) {
                    value = value->NextSiblingElement("value");
                }
            }

            // Добавляет значение хэша при его отсутствии
            if (value == nullptr) {
                value = doc.NewElement("value");
                item->InsertEndChild(value);
                auto newHash = doc.NewText(tmplHash.c_str());
                newHash->SetCData(true);
                value->InsertEndChild(newHash);
            }
        }
        tinydir_next(&dir);
    }

    XMLPrinter printer;
    doc.Print(&printer);
    std::cout << printer.CStr();

    return 0;
}
