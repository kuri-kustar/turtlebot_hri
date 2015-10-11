#include "fdr_server.h"

int main(int argc, char ** argv)
{
   ros::init(argc, argv, "fdr_server");
   std::string img_transp_name = ros::names::append(ros::this_node::getName(), "/image_transport");
   ros::param::set(img_transp_name, "compressed");
   FDRServer fdr_server_1(ros::this_node::getName());
   ros::spin();

   return 0;
}