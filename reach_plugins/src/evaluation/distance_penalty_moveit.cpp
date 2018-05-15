#include <moveit/common_planning_interface_objects/common_objects.h>
#include <moveit/planning_scene/planning_scene.h>
#include <reach_plugins/evaluation/distance_penalty_moveit.h>
#include <xmlrpcpp/XmlRpcException.h>

namespace reach_plugins
{
namespace evaluation
{

DistancePenaltyMoveIt::DistancePenaltyMoveIt()
  : EvaluationBase()
{

}

bool DistancePenaltyMoveIt::initialize(XmlRpc::XmlRpcValue& config)
{
  if(!config.hasMember("planning_group") ||
     !config.hasMember("distance_threshold") ||
     !config.hasMember("exponent"))
  {
    ROS_ERROR("MoveIt Distance Penalty Evaluation plugin is missing one or more configuration parameters");
    return false;
  }

  std::string planning_group;
  try
  {
    planning_group = std::string(config["planning_group"]);
    dist_threshold_ = double(config["distance_threshold"]);
    exponent_ = int(config["exponent"]);
  }
  catch(const XmlRpc::XmlRpcException& ex)
  {
    ROS_ERROR_STREAM(ex.getMessage());
    return false;
  }

  model_ = moveit::planning_interface::getSharedRobotModel("robot_description");
  if(!model_)
  {
    ROS_ERROR("Failed to load robot model");
    return false;
  }

  jmg_ = model_->getJointModelGroup(planning_group);
  if(!jmg_)
  {
    ROS_ERROR("Failed to initialize joint model group pointer");
    return false;
  }

  scene_.reset(new planning_scene::PlanningScene (model_));

  planning_scene_sub_ = nh_.subscribe("update_planning_scene", 1, &DistancePenaltyMoveIt::updatePlanningScene, this);

  return true;
}

double DistancePenaltyMoveIt::calculateScore(const std::vector<double>& pose)
{
  moveit::core::RobotState state (model_);
  state.setJointGroupPositions(jmg_, pose);
  state.update();

  const double dist = scene_->distanceToCollision(state, scene_->getAllowedCollisionMatrix());
  return std::pow((dist / dist_threshold_), exponent_);
}

void DistancePenaltyMoveIt::updatePlanningScene(const moveit_msgs::PlanningSceneConstPtr& msg)
{
  if(!scene_->setPlanningSceneDiffMsg(*msg))
  {
    ROS_ERROR_STREAM("DistancePenaltyMoveIt failed to update planning scene");
  }
}

} // namespace evaluation
} // namespace reach_plugins

#include <pluginlib/class_list_macros.h>
PLUGINLIB_EXPORT_CLASS(reach_plugins::evaluation::DistancePenaltyMoveIt, reach_plugins::evaluation::EvaluationBase)
