#include "fdr_server.h"

int main(int argc, char ** argv)
{
   ros::init(argc, argv, "fdr_server");
   FDRServer fdr_server_1(ros::this_node::getName(), argv[1], argv[2]);
   ros::spin();

   return 0;
}
