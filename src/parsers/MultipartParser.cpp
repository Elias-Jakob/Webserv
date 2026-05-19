#include "MultipartParser.hpp"

MultipartParser::MultipartParser()
{
    std::cout << "MultipartParser built" << std::endl;
}

MultipartParser::~MultipartParser(){}


bool MultipartParser::parse(std::string &body)
{
    std::cout << "parsing MultpartParser..." << std::endl;
    std::cout << body.at(0) << std::endl;
    return true;
}