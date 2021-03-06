/**
 * Copyright (c) 2013 Christian Kerl <christian.kerl@in.tum.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef SERVER_H_
#define SERVER_H_

#include <ros/ros.h>
#include <dynamic_reconfigure/Reconfigure.h>

#include <rdynamic_reconfigure/configuration.h>

namespace rdynamic_reconfigure
{

class Server
{
public:
  typedef boost::function<void (Configuration&)> CallbackType;

  Server(ros::NodeHandle& nh, Configuration& configuration) :
    nh_(nh),
    config_(configuration)
  {
    service_ = nh_.advertiseService("set_parameters", &Server::onServiceCall, this);
    description_publisher_ = nh_.advertise<dynamic_reconfigure::ConfigDescription>("parameter_descriptions", 1, true);
    description_publisher_.publish(configuration.getDescriptionMessage());

    updates_publisher_ = nh_.advertise<dynamic_reconfigure::Config>("parameter_updates", 1, true);

    config_.setDefault();
    config_.fromServer(nh);
    config_.clamp();

    updateConfigInternal();
  }
  virtual ~Server() {};

  void setCallback(const CallbackType& callback)
  {
    callback_ = callback;
    config_.setChanged(true);
    callCallback(config_);
    updateConfigInternal();
  }

  void clearCallback()
  {
    callback_.clear();
  }
private:
  ros::NodeHandle& nh_;
  Configuration& config_;
  ros::ServiceServer service_;
  ros::Publisher description_publisher_, updates_publisher_;
  CallbackType callback_;

  bool onServiceCall(dynamic_reconfigure::Reconfigure::Request& req, dynamic_reconfigure::Reconfigure::Response& res)
  {
    config_.fromMessage(req.config);
    config_.clamp();

    callCallback(config_);

    updateConfigInternal();
    config_.toMessage(res.config);

    return true;
  }

  void callCallback(Configuration& config)
  {
    if (callback_)
    {
      try {
        callback_(config);
      }
      catch (std::exception &e)
      {
        ROS_WARN("Reconfigure callback failed with exception %s: ", e.what());
      }
      catch (...)
      {
        ROS_WARN("Reconfigure callback failed with unprintable exception.");
      }
    }
  }

  void updateConfigInternal()
  {
    dynamic_reconfigure::Config msg;

    config_.setChanged(false);
    config_.toServer(nh_);
    config_.toMessage(msg);
    updates_publisher_.publish(msg);
  }
};

} /* namespace rdynamic_reconfigure */
#endif /* SERVER_H_ */
