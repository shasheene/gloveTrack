#include "gloveTrack.h"

int main(int argc, char** argv){
  if (argc < 2){
    std::cout << argv[0] << " Version " << Glovetrack_VERSION_MAJOR
	 << " " << Glovetrack_VERSION_MINOR << std::endl;
    return 1;
  }


  std::cout << "Hello World \n";
  return (0);
}
