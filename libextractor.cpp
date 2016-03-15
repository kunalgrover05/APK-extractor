///////////////////////////////////////////////////////
//////////// Written by Kunal Grover //////////////////
///////////////////////////////////////////////////////

/* This code takes input an APK or a directory and parses 
   it to print all the Library files(.so) files in each 
   APK file.
   It works by:
    1- Extracting each file from the APK similar to a ZIP
        archive.
    2- For each file process the ELF Header, which tells 
        which architecture it belongs to. This is done since
        the library files are not expected to be in the 
        desired locations by the problem statement. 
        Otherwise, it is a simple trick that library files 
        are always placed as /armeabi/file.so, /x86/file.so etc.
        which allows us to directly get the architecture.
    3- It is difficult to distinguish between 
        armeabi and armeabi-v7 since both of them use the same
        machine type for ELF headers. A possible solution could 
        be using objdump and finding out the corresponding 
        instructions. It is NOT implemented in this solution.

    The output only lists the:
    1- File name
    2- Architecture(32 bit/64 bit)
    3- Machine(Intel(x86)/ARM/MIPS)
*/

// C-dependencies   
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

// C++
#include <iostream>
#include <string>

// External dependencies
#include <zip.h>
#include <libelf.h>
#include <gelf.h>

#define BUFFER_SIZE 10000

using namespace std;

int process_file(const char *);

int main(int argc, const char * argv[])
{
    string help = "-h";
    string directory = "-d";

    // Required to initialize the ELF library
    if (elf_version(EV_CURRENT) == EV_NONE) {
        cout << "Error initializing ELF" << endl;
        return -1;
    }

    // Bring up the help menu and abort other options
    for (int c = 1; c < argc; ++c) {
        if (!help.compare(argv[c])) {
            cout << "You have asked for help\n";
            cout << "Run using ./libextractor <APK1> <APK2> . . \
                    OR ./libextractor -d <DIRECTORY WITH FILES>\n";
            cout << "Dependencies libzip and libelf\n";
            return -1;
        }
    }

    if (!directory.compare(argv[1])) {
        cout << "Taking directory input";
        cout << argv[2];
        DIR *d;
        struct dirent *dir;
        char filename[200];
        d = opendir(argv[2]);
        if (d) {
            while ((dir = readdir(d)) != NULL) {
                struct stat stbuf ;
                sprintf(filename, "%s/%s", argv[2], dir->d_name);
                stat(filename, &stbuf);

                // Process non directories
                if ((stbuf.st_mode & S_IFMT) != S_IFDIR ) {
                    process_file(filename);
                }
            }
            closedir(d);
        }
    }
    else {
        for (int c = 1; c < argc; ++c) {
            process_file(argv[c]);
        }
    }
}

int process_file(const char* filename) {
    // Initializations
    int i, r, count, p;
    char buffer[BUFFER_SIZE];
    char tempfile_name[] = "tempfile";

    string name;

    Elf *e;
    Elf_Kind ek;
    GElf_Ehdr ehdr;

    // Using C-style file pointer, due to easy interchangability
    // with Integer file descriptors
    FILE * fp;

    struct zip_stat statFile;
    zip_file *fileToExtract;

    cout << "Processing " << filename << "\n";
    // Open the APK file and check if there is an error
    int err;
    zip *archive = zip_open(filename, 0, &err);
    if (err != ZIP_ER_OK) {
        cout << "Error " << err << endl;
        return -1;
    }

    count = 0;
    i = 0;

    // Iterate through each file in archive
    while (i < zip_get_num_files(archive)) {
        zip_stat_index(archive, i, 0, &statFile);
        name = zip_get_name(archive, i, 0);

        // Process if it is a file and not if it is a folder
        if (name[name.size() - 1] != '/') {
            fileToExtract = zip_fopen_index(archive, i, 0);

            // Extract to a temporary file
            fp = fopen (tempfile_name, "wb");
            while ((r = zip_fread(fileToExtract, buffer, sizeof(buffer))) > 0) {
                fwrite(buffer, sizeof(char), BUFFER_SIZE, fp);
            };
            fclose (fp);

            // Now Read the file
            fp = fopen (tempfile_name, "r");
            e = elf_begin(fileno(fp) , ELF_C_READ, NULL);
            if (elf_kind(e) == ELF_K_ELF) {
                // It is an ELF file
                gelf_getehdr (e , & ehdr);
                p = gelf_getclass(e);
                if ( p != ELFCLASSNONE ) {
                    // Finally extract Machine type from the ELF HEADER
                    cout << name << "\t";
                    cout << ((p == 2) ? 64 : 32) << " bit\t";

                    if (ehdr.e_machine == 0x28) cout << "ARM";
                    else if (ehdr.e_machine == 0x3) cout << "Intel";
                    else if (ehdr.e_machine == 0x8) cout << "MIPS";
                    else cout << "Unknown";
                    cout << endl;

                    count += 1;
                }
            }
            elf_end(e);
            fclose(fp);
            zip_fclose(fileToExtract);
        }
        i++;
    }
    cout << count << " Library files\n\n";
    zip_close(archive);
    return 0;
}
