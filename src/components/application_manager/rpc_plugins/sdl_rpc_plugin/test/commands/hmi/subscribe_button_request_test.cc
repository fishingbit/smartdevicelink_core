/*
 * Copyright (c) 2021, Ford Motor Company
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided with the
 * distribution.
 *
 * Neither the name of the Ford Motor Company nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "hmi/subscribe_button_request.h"
#include <memory>
#include <string>
#include "application_manager/mock_event_dispatcher.h"

#include "gtest/gtest.h"

namespace test {
namespace components {
namespace commands_test {
namespace hmi_commands_test {
namespace subscribe_button_request {

using ::testing::_;
using ::testing::Return;
namespace am = ::application_manager;
namespace strings = am::strings;
using am::commands::RequestToHMI;
using am::event_engine::Event;
using sdl_rpc_plugin::commands::hmi::SubscribeButtonRequest;
using ::test::components::application_manager_test::MockApplication;
using ::test::components::event_engine_test::MockEventDispatcher;

typedef std::shared_ptr<MockApplication> MockAppPtr;
typedef std::shared_ptr<RequestToHMI> RequestToHMIPtr;
typedef std::shared_ptr<SubscribeButtonRequest> SubscribeButtonRequestPtr;

namespace {
const uint32_t kCorrelationId = 2u;
const uint32_t kAppId = 1u;
const hmi_apis::Common_ButtonName::eType kCustomButtonName =
    hmi_apis::Common_ButtonName::CUSTOM_BUTTON;
const hmi_apis::Common_ButtonName::eType kHmiButtonName =
    hmi_apis::Common_ButtonName::SEEKLEFT;
const mobile_apis::ButtonName::eType kMobileButtonName =
    mobile_apis::ButtonName::SEEKLEFT;
const mobile_apis::ButtonName::eType kMobileCustomButtonName =
    mobile_apis::ButtonName::CUSTOM_BUTTON;
const hmi_apis::FunctionID::eType kFunctionID =
    hmi_apis::FunctionID::Buttons_SubscribeButton;
}  // namespace

class HMISubscribeButtonRequestTest
    : public CommandRequestTest<CommandsTestMocks::kIsNice> {
 protected:
  MessageSharedPtr CreateCommandMsg() {
    MessageSharedPtr command_msg(CreateMessage(smart_objects::SmartType_Map));
    (*command_msg)[strings::msg_params][strings::app_id] = kAppId;
    (*command_msg)[strings::msg_params][strings::button_name] = kHmiButtonName;
    (*command_msg)[strings::params][strings::correlation_id] = kCorrelationId;
    (*command_msg)[strings::params][strings::function_id] = kFunctionID;

    return command_msg;
  }

  void InitCommand(const uint32_t& timeout) OVERRIDE {
    mock_app_ = CreateMockApp();
    CommandRequestTest<CommandsTestMocks::kIsNice>::InitCommand(timeout);
    ON_CALL((*mock_app_), hmi_app_id()).WillByDefault(Return(kAppId));
    ON_CALL(app_mngr_, application_by_hmi_app(kAppId))
        .WillByDefault(Return(mock_app_));
    ON_CALL(app_mngr_, application(kAppId)).WillByDefault(Return(mock_app_));
  }

  MockAppPtr mock_app_;
};

TEST_F(HMISubscribeButtonRequestTest, Run_SendRequest_SUCCESS) {
  MockEventDispatcher mock_event_dispatcher;
  EXPECT_CALL(app_mngr_, event_dispatcher())
      .WillOnce(ReturnRef(mock_event_dispatcher));

  MessageSharedPtr command_msg = CreateCommandMsg();
  RequestToHMIPtr command(CreateCommand<SubscribeButtonRequest>(command_msg));

  EXPECT_CALL(mock_event_dispatcher,
              add_observer(kFunctionID, kCorrelationId, _))
      .Times(0);
  EXPECT_CALL(mock_rpc_service_, SendMessageToHMI(command_msg));
  ASSERT_TRUE(command->Init());

  command->Run();
}

TEST_F(HMISubscribeButtonRequestTest, Run_SendRequest_CUSTOM_BUTTON_SUCCESS) {
  MockEventDispatcher mock_event_dispatcher;
  EXPECT_CALL(app_mngr_, event_dispatcher())
      .WillOnce(ReturnRef(mock_event_dispatcher));

  MessageSharedPtr command_msg = CreateCommandMsg();
  (*command_msg)[strings::msg_params][strings::button_name] = kCustomButtonName;
  RequestToHMIPtr command(CreateCommand<SubscribeButtonRequest>(command_msg));

  EXPECT_CALL(mock_event_dispatcher,
              add_observer(kFunctionID, kCorrelationId, _));
  EXPECT_CALL(mock_rpc_service_, SendMessageToHMI(command_msg));

  ASSERT_TRUE(command->Init());
  command->Run();
}

TEST_F(HMISubscribeButtonRequestTest,
       onTimeOut_RequestIsExpired_HandleOnTimeout) {
  MessageSharedPtr command_msg = CreateCommandMsg();
  RequestToHMIPtr command(CreateCommand<SubscribeButtonRequest>(command_msg));

  resumption_test::MockResumeCtrl mock_resume_ctrl;
  ON_CALL(app_mngr_, resume_controller())
      .WillByDefault(ReturnRef(mock_resume_ctrl));
  EXPECT_CALL(mock_resume_ctrl, HandleOnTimeOut(kCorrelationId, kFunctionID));

  command->OnTimeOut();
}

TEST_F(HMISubscribeButtonRequestTest,
       OnEvent_SuccessfulResponse_ButtonSubscribed) {
  MessageSharedPtr command_msg = CreateCommandMsg();
  (*command_msg)[strings::msg_params][strings::button_name] = kCustomButtonName;

  SubscribeButtonRequestPtr command =
      CreateCommand<SubscribeButtonRequest>(command_msg);

  MessageSharedPtr event_msg = CreateCommandMsg();
  (*event_msg)[am::strings::params][am::hmi_response::code] =
      hmi_apis::Common_Result::SUCCESS;

  Event event(kFunctionID);
  event.set_smart_object(*event_msg);

  EXPECT_CALL(*mock_app_, SubscribeToButton(kMobileCustomButtonName))
      .WillOnce(Return(true));

  command->on_event(event);
}

TEST_F(HMISubscribeButtonRequestTest,
       OnEvent_UnsuccessfulResponse_ButtonNotSubscribed) {
  MessageSharedPtr command_msg = CreateCommandMsg();
  (*command_msg)[strings::msg_params][strings::button_name] = kCustomButtonName;

  SubscribeButtonRequestPtr command =
      CreateCommand<SubscribeButtonRequest>(command_msg);

  MessageSharedPtr event_msg = CreateCommandMsg();
  (*event_msg)[am::strings::params][am::hmi_response::code] =
      hmi_apis::Common_Result::GENERIC_ERROR;

  Event event(kFunctionID);
  event.set_smart_object(*event_msg);

  EXPECT_CALL(*mock_app_, SubscribeToButton(kMobileCustomButtonName)).Times(0);

  command->on_event(event);
}

TEST_F(HMISubscribeButtonRequestTest, onEvent_App_NULL) {
  MessageSharedPtr command_msg = CreateCommandMsg();
  SubscribeButtonRequestPtr command =
      CreateCommand<SubscribeButtonRequest>(command_msg);

  MessageSharedPtr event_msg = CreateCommandMsg();
  (*event_msg)[am::strings::params][am::hmi_response::code] =
      hmi_apis::Common_Result::SUCCESS;

  Event event(kFunctionID);
  event.set_smart_object(*event_msg);

  MockAppPtr mock_app = NULL;
  EXPECT_CALL(app_mngr_, application_by_hmi_app(kAppId))
      .WillOnce(Return(mock_app));

  EXPECT_CALL(*mock_app_, SubscribeToButton(kMobileButtonName)).Times(0);

  command->on_event(event);
}

}  // namespace subscribe_button_request
}  // namespace hmi_commands_test
}  // namespace commands_test
}  // namespace components
}  // namespace test
