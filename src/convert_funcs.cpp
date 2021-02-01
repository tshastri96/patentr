#include <Rcpp.h>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>

using namespace Rcpp;

// checks if string starts w/ a prefix
// WORKS (basic test cases)
bool startsWith(const std::string &text, const std::string &prefix)
{
    return (text.find(prefix) == 0);
}

// remove whitespace from beginning of a string
// WORKS (basic test cases)
void stripBeginWhitespace(std::string &text)
{
    int start = 0;
    while (text[start] == ' ' || text[start] == '\n' || text[start] == '\t' || text[start] == '\r')
        start++;
    text = text.substr(start);
}

// remove whitespace from end of a string
// WORKS (basic test cases)
void stripEndWhitespace(std::string &text)
{
    int end = text.length();
    while (text[end - 1] == ' ' || text[end - 1] == '\n' || text[end - 1] == '\t' || text[end - 1] == '\r')
      end--;
    text = text.substr(0, end);
}

// remove whitespace from beginning and end
void stripEdgeWhitespace(std::string &text)
{
    stripBeginWhitespace(text);
    stripEndWhitespace(text);
}

// extract single-line field and strip whitespace
std::string extractField(const std::string &line, int startPos)
{
    // extract, strip whitespace, and return
    std::string ans = line.substr(startPos);
    stripEdgeWhitespace(ans);
    return ans;
}

void formatName(std::string &name)
{
    int semicolon = name.find(';');

    // leave as-is if there's no semi colon or if the format is not as expected
    if (semicolon == std::string::npos || name.length() < semicolon + 2) return ;

    std::string temp = name.substr(0, semicolon - 1),
                revised = name.substr(semicolon + 2);

    name = revised + " " + temp;
}

void appendToField(std::string &orig, const std::string &addon)
{
    if (orig == "")
      orig = addon;
    else
      orig = orig + ';' + addon;
}

// [[Rcpp::export]]
int txt_to_df_cpp(std::string input_file, std::string output_file)
{
    // setup IO
    std::ifstream fin(input_file);
    std::ofstream fout(output_file);

    // output header line to CSV
    fout << "WKU,Title,App_Date,Issue_Date,Inventor,Assignee,ICL_Class,References\n";

    // variables holding patent properties
    std::string currID = "",
                title = "",
                appDate = "",
                issDate = "",
                inventor = "",
                assignee = "",
                iclClass = "",
                refs = "",
                currLine,
                tempLine,
                tempInvt = "";
    bool inPatent = false,
         gotAPD = false,
         gotISD = false;

    // read input file line-by-line and store patent data
    getline(fin, currLine);
    int countPat = 0;
    while (!fin.eof())
    {
        // look at current line
        if (startsWith(currLine, "PATN"))
        {
            // print past patent (unless this is the first one)
            if (inPatent)
            {
                fout << currID
                  << ",\"" << title
                  << "\"," << appDate
                  << "," << issDate
                  << ",\"" << inventor
                  << "\"," << assignee
                  << "," << iclClass
                  << "," << refs
                  << "\n";
            }
            else inPatent = true;

            // update counter/tracker vars
            countPat++;
            gotAPD = false;
            tempInvt = "";
            inventor = "";
        }
        else if (inPatent && startsWith(currLine, "TTL  "))
        {
            title = extractField(currLine, 5);
        }
        else if (inPatent && startsWith(currLine, "WKU  "))
        {
            currID = extractField(currLine, 5);
        }
        else if (inPatent && !gotAPD && startsWith(currLine, "APD  "))
        {
            gotAPD = true;
            appDate = extractField(currLine, 5);
        }
        else if (inPatent && !gotISD && startsWith(currLine, "ISD  "))
        {
            gotISD = true;
            issDate = extractField(currLine, 5);
        }
        else if (inPatent && startsWith(currLine, "INVT"))
        {
            // read next line to get inventor name (and confirm format)
            getline(fin, tempLine);
            if (startsWith(tempLine, "NAM  "))
            {
                tempInvt = extractField(tempLine, 5);
                formatName(tempInvt);
            }

            // add this inventor to set of inventors for this patent
            appendToField(inventor, tempInvt);
        }

        // read next line
        getline(fin, currLine);
    }

    // output details of last patent
    fout << currID
         << ",\"" << title
         << "\"," << appDate
         << "," << issDate
         << "," << inventor
         << "," << assignee
         << "," << iclClass
         << "," << refs
         << "\n";

    // close IO
    fin.close();
    fout.close();

    // return number of patents
    return countPat;
}
