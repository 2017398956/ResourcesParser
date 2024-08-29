#include <stdlib.h>
#include <string>
#include <algorithm>
#include "ParseArsc.h"
#include "ParseXML.h"

using namespace std;

bool endsWith(std::string const &str, std::string const &suffix) {
    if (str.length() < suffix.length()) {
        return false;
    }
    return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}

int main(int argc, char *argv[])
{
    std::string str = string(argv[1]);
    std::string arsc = "arsc";
    std::string xml = "xml";
    if (endsWith(str, arsc))
    {
        parseArsc(str.c_str());
    }
    else if (endsWith(str, xml))
    {
        parseXml(str.c_str());
    }
    else {
        printf("请输入 arsc 文件，或 xml 文件！");
    }
	return 0;
}