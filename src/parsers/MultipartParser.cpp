#include "MultipartParser.hpp"

MultipartParser::MultipartParser()
{
    std::cout << "MultipartParser constructed" << std::endl;
}

MultipartParser::~MultipartParser(){}

bool MultipartParser::parse(std::string &body)
{
    std::cout << "parsing MultpartParser..." << std::endl;
    // std::cout << "check for boundary: " << _contentData.
    std::cout << body.at(0) << std::endl;
    // size_t posBoundary = 0;
    // std::string fileInput;
    // posBoundary = body.find(, 0, body.size());
    return true;
}