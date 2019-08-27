#include "shelpers.hpp"

/*
 text handling functions
 */

bool splitOnSymbol(std::vector<std::string>& words, int i, char c)
{
    if(words[i].size() < 2) { return false; }
    int pos;
    if((pos = words[i].find(c)) != std::string::npos){
        if(pos == 0)
        {
            //starts with symbol
            words.insert(words.begin() + i + 1, words[i].substr(1, words[i].size() -1));
            words[i] = words[i].substr(0,1);
        }
        else
        {
            //symbol in middle or end
            words.insert(words.begin() + i + 1, std::string{c});
            std::string after = words[i].substr(pos + 1, words[i].size() - pos - 1);
            if(!after.empty())
            {
                words.insert(words.begin() + i + 2, after);
            }
            words[i] = words[i].substr(0, pos);
        }
        return true;
    } else {
        return false;
    }
    
}

std::vector<std::string> tokenize(const std::string& s)
{
    std::vector<std::string> ret;
    int pos = 0;
    int space;
    //split on spaces
    while((space = s.find(' ', pos)) != std::string::npos)
    {
        std::string word = s.substr(pos, space - pos);
        if(!word.empty())
        {
            ret.push_back(word);
        }
        pos = space + 1;
    }
    
    std::string lastWord = s.substr(pos, s.size() - pos);
    if(!lastWord.empty())
    {
        ret.push_back(lastWord);
    }
    
    for(int i = 0; i < ret.size(); ++i)
    {
        for(auto c : {'&', '<', '>', '|'})
        {
            if(splitOnSymbol(ret, i, c))
            {
                --i;
                break;
            }
        }
    }
    
    return ret;
    
}


std::ostream& operator<<(std::ostream& outs, const Command& c)
{
    outs << c.exec << " argv: ";
    for(const auto& arg : c.argv){ if(arg) {outs << arg << ' ';}}
    outs << "fds: " << c.fdStdin << ' ' << c.fdStdout << ' ' << (c.background ? "background" : "");
    return outs;
}

//returns an empty vector on error

std::vector<Command> getCommands(const std::vector<std::string>& tokens)
{
    std::vector<Command> ret(std::count(tokens.begin(), tokens.end(), "|") + 1);  //1 + num |'s commands
    
    int first = 0;
    int last = std::find(tokens.begin(), tokens.end(), "|") - tokens.begin();
    bool error = false;
    for(int i = 0; i < ret.size(); ++i)
    {
        if((tokens[first] == "&") || (tokens[first] == "<") ||
           (tokens[first] == ">") || (tokens[first] == "|"))
        {
            error = true;
            break;
        }
        std::cout << ret.size();
        ret[i].exec = tokens[first];
        ret[i].argv.push_back(tokens[first].c_str());
        ret[i].fdStdin = 0;
        ret[i].fdStdout = 1;
        ret[i].background = false;
        
        for(int j = first + 1; j < last; ++j)
        {
            if(tokens[j] == ">" || tokens[j] == "<" ){
                if(tokens[j] == ">")
                {
                    int out;
                    if((out = open(tokens[j+1].c_str(), O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR, 0666)) < 0)
                    {
                        perror("fdStdout failure.");
                        EXIT_FAILURE;
                    }
                    ret[i].fdStdout = out;
                    ++j;
                }
                else
                {
                    int in;
                    if((in = open(tokens[j+1].c_str(), O_RDONLY)) < 0)
                    {
                        perror("fdStdin failure.");
                        EXIT_FAILURE;
                    }
                    ret[i].fdStdin = in;
                    ++j;
                }
                
            }
            else if(tokens[j] == "&")
            {
                //Fill this in if you choose to do the optional "background command" part
                assert(false);
            }
            else
            {
                //otherwise this is a normal command line argument!
                ret[i].argv.push_back(tokens[j].c_str());
            }
            
        }
        if(i > 0)
        {
            int fds[2];
            if (pipe(fds))
            {
                perror("Pipe failure.");
                EXIT_FAILURE;
            }
            ret[i].fdStdin = fds[0];
            if(ret[i-1].fdStdout == 1)
            {
                ret[i-1].fdStdout = fds[1];
            }
            
            /* there are multiple commands.  Open open a pipe and
             Connect the ends to the fds for the commands!
             */
        }
        //exec wants argv to have a nullptr at the end!
        ret[i].argv.push_back(nullptr);
        
        //find the next pipe character
        first = last + 1;
        if(first < tokens.size())
        {
            last = std::find(tokens.begin() + first, tokens.end(), "|") - tokens.begin();
        }
    }

    if(error)
    {
        for(int e = 0; e < ret.size(); ++e)
        {
            if(ret[e].fdStdin != 0)
            {
                close(ret[e].fdStdin);
            }
            if(ret[e].fdStdout != 1)
            {
                close(ret[e].fdStdout);
            }
        }
    }
    
    return ret;
}

// additional built in c function for getting usr https://linux.die.net/man/3/getlogin_r
void printPrompt()
{
    char *buf;
    buf=(char *)malloc(10*sizeof(char));
    if(getlogin_r(buf,10) != 0)
    {
        std::cerr << "username error";
    }
    printf("%s $ ", buf);
}
