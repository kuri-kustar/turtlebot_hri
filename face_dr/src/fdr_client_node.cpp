#include "fdr_client.h"

int main(int argc, char ** argv)
{
   ros::init(argc, argv, "fdr_client");
   FDRClient fdr_client_1(argv[1], argv[2], argv[3]);
   ros::spin();

   return 0;
}
