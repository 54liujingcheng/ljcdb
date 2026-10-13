#include "execute.h"
int main(int argc, char* argv[]){
    MyBitMap::initConst();
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    bool mode = false;
    std::string file_path = "";
    std::string table_name = "";
    std::string db_name = "";
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-b" || arg == "--batch"){
            mode = true;
        } 
        else if (arg == "-f" || arg == "--file"){
            if (i + 1 < argc)file_path = argv[++i];
            else{
                std::cerr << "Error: option -f requires a path argument.\n";
                return 1;
            }
        } 
        else if (arg == "-t" || arg == "-table"){
            if (i + 1 < argc)table_name = argv[++i];
            else{
                std::cerr << "Error: option -t requires an argument.\n";
                return 1;
            }
        } 
        else if (arg == "-d" || arg == "--database"){
            if (i + 1 < argc)db_name = argv[++i];
            else{
                std::cerr << "Error: option -d requires an argument.\n";
                return 1;
            }
        } 
        else if(arg == "--init"){
            std::error_code ec;
            string dir = "data";
            if (fs::exists(dir, ec)) {
                if (!fs::is_directory(dir, ec)) {
                    return 1;
                }
                for (const auto& entry : fs::directory_iterator(dir, ec)) {
                    fs::remove_all(entry.path(), ec);
                    if (ec) {
                        return 1;
                    }
                }
            } else {
                fs::create_directories(dir, ec);
                if (ec) {
                    return 1;
                }
            }
            return 0;
        }
        else printf("Warning: unknown option %s\n", arg.c_str());
    }
    LoadRequest load;load.file_name = file_path, load.table_name = table_name;
    SimpleRequest use;use.command = Simple::USE_DATABASE, use.name = db_name;
    DbManager* dbm = new DbManager();
    if(db_name != ""){
        int res = dbm->UseDb(db_name);
        if(res == 0 || res == -2){
            if(mode == 1)printf("@\n");
            else if(mode == 0&&res == 0)printf("Switching into this datebase.\n");
            else if(mode == 0&&res == -2)printf("You have been using this datebase so far.\n");
        }else if(res == -1){
            if(mode == 1)printf("!ERROR\nelse\n@\n");
            else if(mode == 0)printf("Fail in using the datebase.\n");
        }
        fflush(stdout);
    }
    if(file_path != "" && table_name != ""){
        int res = LoadCSV(*dbm, table_name, file_path);
        if(mode==1)printf("rows\n%d\n@\n",res);
        else if(mode==0)printf("Load %d rows into %s.\n", res, table_name.c_str());
        fflush(stdout);
    }
    string command,line;
    while(1){
        if(mode == 0){
            if(command.empty())printf("ljcdb:%s>", dbm->dbname.c_str());
            else printf(">");
            fflush(stdout);
        }
        if(!getline(cin,line))break;
        if(line.size() >= 2 && line[0] == '-'&&line[1] == '-')continue;
        while(!line.empty()&&line.back() == ' ')line.pop_back();
        command += line;
        if(line.empty()){
            if(mode == 0){
                printf(">");
                fflush(stdout);
            }
            command += " ";
        }
        else if(line == "exit" || line == "EXIT"){
            break;
        }
        else if(line.back() == ';'){
            try {
                runSQLJC(*dbm, command, mode);
            }
            catch (const std::exception& e) {
                if (mode) {
                    printf("!ERROR\n");
                    printf("else\n");
                    printf("@\n");
                }
                else {
                    printf("SQL error: %s\n", e.what());
                }
                fflush(stdout);
            }
            catch (...) {
                if (mode)
                    printf("!ERROR\nelse\n@\n");
                else
                    printf("Unknown SQL error.\n");
                fflush(stdout);
            }
            command.clear();
            command.clear();
        }
    }
    delete dbm;
}