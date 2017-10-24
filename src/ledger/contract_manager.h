/*
Copyright Bubi Technologies Co., Ltd. 2017 All Rights Reserved.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef CONTRACT_MANAGER_H_
#define CONTRACT_MANAGER_H_
#include <map>
#include <unordered_map>
#include <string>

#include <utils/headers.h>
#include <v8.h>
#include <libplatform/libplatform.h>
#include <libplatform/libplatform-export.h>
#include <proto/cpp/chain.pb.h>
#include "ledgercontext_manager.h"

namespace bubi{

	class ContractParameter {
	public:
		ContractParameter();
		~ContractParameter();

		std::string code_;
		std::string input_;
		std::string this_address_;
		std::string sender_;
		std::string trigger_tx_;
		int32_t ope_index_;
		std::string consensus_value_;
		LedgerContext *ledger_context_;
	};

	class Contract {
	protected:
		int32_t type_;
		int64_t id_;
		ContractParameter parameter_;
		std::string error_msg_;
		int32_t tx_do_count_;  //transactions trigger by one contract
	public:
		Contract();
		Contract(const ContractParameter &parameter);
		virtual ~Contract();

	public:
		virtual bool Execute();
		virtual bool Cancel();
		virtual bool SourceCodeCheck();
		virtual bool Query(Json::Value& jsResult);

		int32_t GetTxDoCount();
		void IncTxDoCount();
		int64_t GetId();
		ContractParameter &GetParameter();
		std::string GetErrorMsg();
		static utils::Mutex contract_id_seed_lock_;
		static int64_t contract_id_seed_;

		enum TYPE {
			TYPE_V8 = 0,
			TYPE_ETH = 1
		};
	};

	class V8Contract : public Contract {
		v8::Isolate* isolate_;
		v8::Global<v8::Context> g_context_;
	public:
		V8Contract(const ContractParameter &parameter);
		virtual ~V8Contract();
	public:
		virtual bool Execute();
		virtual bool Cancel();
		virtual bool Query(Json::Value& jsResult);
		virtual bool SourceCodeCheck();

		static bool Initialize(int argc, char** argv);
		static bool LoadJsLibSource();
		static std::map<std::string, std::string> jslib_sources;
		static const std::string sender_name_;
		static const std::string this_address_;
		static const char* main_name_;
		static const char* query_name_;
		static const std::string trigger_tx_name_;
		static const std::string trigger_tx_index_name_;
		static const std::string this_header_name_;

		static utils::Mutex isolate_to_contract_mutex_;
		static std::unordered_map<v8::Isolate*, V8Contract *> isolate_to_contract_;

		static v8::Platform* 	platform_;
		static v8::Isolate::CreateParams create_params_;

		v8::Local<v8::Context> CreateContext(v8::Isolate* isolate);
		static V8Contract *GetContractFrom(v8::Isolate* isolate);
		static std::string ReportException(v8::Isolate* isolate, v8::TryCatch* try_catch);
		static const char* ToCString(const v8::String::Utf8Value& value);
		static void CallBackLog(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void CallBackGetAccountMetaData(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void CallBackSetAccountMetaData(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void CallBackGetAccountAsset(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void Include(const v8::FunctionCallbackInfo<v8::Value>& args);
		//get account info from an account
		static void CallBackGetAccountInfo(const v8::FunctionCallbackInfo<v8::Value>& args);
		//get a ledger info from a ledger
		static void CallBackGetLedgerInfo(const v8::FunctionCallbackInfo<v8::Value>& args);
		//get transaction info from a transaction
		static void CallBackGetTransactionInfo(const v8::FunctionCallbackInfo<v8::Value>& args);
		//static void CallBackGetThisAddress(const v8::FunctionCallbackInfo<v8::Value>& args);
		//make a transaction
		static void CallBackDoOperation(const v8::FunctionCallbackInfo<v8::Value>& args);
		static V8Contract *UnwrapContract(v8::Local<v8::Object> obj);
		static bool JsValueToCppJson(v8::Handle<v8::Context>& context, v8::Local<v8::Value>& jsvalue, std::string& key, Json::Value& jsonvalue);
	};

	class QueryContract : public utils::Thread{
		Contract *contract_;
		ContractParameter parameter_;
		Json::Value result_;
		bool ret_;
		utils::Mutex mutex_;
	public:
		QueryContract();
		~QueryContract();

		bool Init(int32_t type, const std::string &code, const std::string &input);
		virtual void Run();
		void Cancel();
		bool GetResult(Json::Value &result);
	};

	typedef std::map<int64_t, Contract *> ContractMap;
	class ContractManager :
		public utils::Singleton<ContractManager>{
		friend class utils::Singleton<ContractManager>;

		utils::Mutex contracts_lock_;
		ContractMap contracts_;
	public:
		ContractManager();
		~ContractManager();

		bool Initialize(int argc, char** argv);
		bool Exit();

		bool Execute(int32_t type, const ContractParameter &paramter, std::string &error_msg);
		bool Cancel(int64_t contract_id);
		bool SourceCodeCheck(int32_t type, const std::string &code, std::string &error_msg);
		bool Query(int32_t type, const std::string &code, const std::string &input, Json::Value& jsResult);
		Contract *GetContract(int64_t contract_id);
	};
}
#endif