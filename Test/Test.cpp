#include "pch.h"
#include "CppUnitTest.h"
#include "../registry.h"
#include <vector>
#include <functional>
#include <Windows.h>

#define CONST_STR inline static const char * const

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

const std::vector<HKEY> machines = {
	//HKEY_CLASSES_ROOT,	// requires admin
	HKEY_CURRENT_USER,
	//HKEY_LOCAL_MACHINE,	// cannot create keys even using regedit
	//HKEY_USERS,		// cannot create keys even using regedit
	//HKEY_CURRENT_CONFIG	// requires admin
};

void test_with(std::function<void(HKEY)> foo, std::vector<HKEY> values = machines)
{
	for (const HKEY& machine : values)
		foo(machine);
}

void createNKeys(HKEY machine, std::string_view key, size_t count)
{
	for (size_t i = 0; i < count; i++)
		reg::create::key(machine, reg::except::concat_string(key, "\\", i));
}

void createNValues(HKEY machine, std::string_view key, size_t count)
{
	for (size_t i = 0; i < count; i++)
	{
		if (i % 2)
			reg::create::number(machine, key, reg::except::concat_string("Value", i), i * 2);
		else
			reg::create::string(machine, key, reg::except::concat_string("Value", i), "This is a string");
	}
}

#define IMMEDIATE_KEY_ROOT "TestKey"
#define SHALLOW_KEY_ROOT "ShallowKey"
#define DEEP_KEY_ROOT "DeepKey"

CONST_STR immediate_key = IMMEDIATE_KEY_ROOT;
CONST_STR shallow_key = SHALLOW_KEY_ROOT"\\Level1\\Level2";
CONST_STR deep_key = DEEP_KEY_ROOT"\\Level1\\Level2\\Level3\\"
"Level4\\Level5\\Level6\\Level7\\"
"Level8\\Level9\\Level10";

CONST_STR value_str_name = "MyString";
CONST_STR value_num_name = "MyNumber";

namespace Util
{
	TEST_CLASS(Key_Exists)
	{
	public:
		TEST_METHOD(Key_Does_Exist)
		{
			test_with([](HKEY machine) {
				reg::create::key(machine, immediate_key);
				reg::create::key(machine, shallow_key);
				reg::create::key(machine, deep_key);
				// -- setup

				Assert::IsTrue(reg::key_exists(machine, immediate_key));
				Assert::IsTrue(reg::key_exists(machine, shallow_key));
				Assert::IsTrue(reg::key_exists(machine, deep_key));

				// cleanup
				reg::remove::cluster(machine, IMMEDIATE_KEY_ROOT);
				reg::remove::cluster(machine, SHALLOW_KEY_ROOT);
				reg::remove::cluster(machine, DEEP_KEY_ROOT);
				});
		}
		TEST_METHOD(Key_Does_Not_Exist)
		{
			test_with([](HKEY machine) {
				reg::remove::cluster(machine, IMMEDIATE_KEY_ROOT);
				reg::remove::cluster(machine, SHALLOW_KEY_ROOT);
				reg::remove::cluster(machine, DEEP_KEY_ROOT);
				// -- setup

				Assert::IsFalse(reg::key_exists(machine, immediate_key));
				Assert::IsFalse(reg::key_exists(machine, shallow_key));
				Assert::IsFalse(reg::key_exists(machine, deep_key));
				});
		}
	};

	TEST_CLASS(Value_Exists)
	{
	public:
		TEST_METHOD(Value_Does_Exist)
		{
			test_with([](HKEY machine) {
				reg::create::number(machine, "", value_num_name);
				reg::create::number(machine, immediate_key, value_num_name);
				reg::create::number(machine, shallow_key, value_num_name);
				reg::create::number(machine, deep_key, value_num_name);
				// -- setup

				Assert::IsTrue(reg::value_exists(machine, "", value_num_name));
				Assert::IsTrue(reg::value_exists(machine, immediate_key, value_num_name));
				Assert::IsTrue(reg::value_exists(machine, shallow_key, value_num_name));
				Assert::IsTrue(reg::value_exists(machine, deep_key, value_num_name));

				// cleanup
				reg::remove::value(machine, "", value_num_name);
				reg::remove::cluster(machine, IMMEDIATE_KEY_ROOT);
				reg::remove::cluster(machine, SHALLOW_KEY_ROOT);
				reg::remove::cluster(machine, DEEP_KEY_ROOT);
				});
		}
		TEST_METHOD(Value_Does_Not_Exist)
		{
			test_with([](HKEY machine) {
				reg::remove::value(machine, "", value_num_name);
				reg::remove::cluster(machine, IMMEDIATE_KEY_ROOT);
				reg::remove::cluster(machine, SHALLOW_KEY_ROOT);
				reg::remove::cluster(machine, DEEP_KEY_ROOT);
				// -- setup

				Assert::IsFalse(reg::value_exists(machine, "", value_num_name));
				Assert::IsFalse(reg::value_exists(machine, immediate_key, value_num_name));
				Assert::IsFalse(reg::value_exists(machine, shallow_key, value_num_name));
				Assert::IsFalse(reg::value_exists(machine, deep_key, value_num_name));
				});
		}
	};

	TEST_CLASS(Peek_Value)
	{
	public:
		TEST_METHOD(Key_Does_Exists_Value_Does_Not_Exist)
		{
			test_with([](HKEY machine) {
				reg::remove::cluster(machine, IMMEDIATE_KEY_ROOT);
				reg::remove::cluster(machine, SHALLOW_KEY_ROOT);
				reg::remove::cluster(machine, DEEP_KEY_ROOT);

				reg::create::key(machine, immediate_key);
				reg::create::key(machine, shallow_key);
				reg::create::key(machine, deep_key);
				// -- setup

				Assert::ExpectException<reg::except::value_not_found>([machine]() {reg::peekvalue(machine, "", value_num_name); });
				Assert::ExpectException<reg::except::value_not_found>([machine]() {reg::peekvalue(machine, immediate_key, value_num_name); });
				Assert::ExpectException<reg::except::value_not_found>([machine]() {reg::peekvalue(machine, shallow_key, value_num_name); });
				Assert::ExpectException<reg::except::value_not_found>([machine]() {reg::peekvalue(machine, deep_key, value_num_name); });

				// cleanup
				reg::remove::cluster(machine, IMMEDIATE_KEY_ROOT);
				reg::remove::cluster(machine, SHALLOW_KEY_ROOT);
				reg::remove::cluster(machine, DEEP_KEY_ROOT);
				});
		}

		TEST_METHOD(Key_Does_Not_Exist)
		{
			test_with([](HKEY machine) {
				reg::remove::cluster(machine, IMMEDIATE_KEY_ROOT);
				reg::remove::cluster(machine, SHALLOW_KEY_ROOT);
				reg::remove::cluster(machine, DEEP_KEY_ROOT);
				// -- setup

				Assert::ExpectException<reg::except::key_not_found>([machine]() {reg::peekvalue(machine, immediate_key, value_num_name); });
				Assert::ExpectException<reg::except::key_not_found>([machine]() {reg::peekvalue(machine, shallow_key, value_num_name); });
				Assert::ExpectException<reg::except::key_not_found>([machine]() {reg::peekvalue(machine, deep_key, value_num_name); });
				});
		}

		TEST_METHOD(Value_Is_Number)
		{
			test_with([](HKEY machine) {
				reg::remove::value(machine, "", value_num_name);
				reg::remove::cluster(machine, IMMEDIATE_KEY_ROOT);
				reg::remove::cluster(machine, SHALLOW_KEY_ROOT);
				reg::remove::cluster(machine, DEEP_KEY_ROOT);

				reg::create::number(machine, "", value_num_name, 1);
				reg::create::number(machine, immediate_key, value_num_name, 1234);
				reg::create::number(machine, shallow_key, value_num_name, MAXDWORD);
				reg::create::number(machine, deep_key, value_num_name, -1);
				// -- setup

				auto [type1, size1] = reg::peekvalue(machine, "", value_num_name);
				auto [type2, size2] = reg::peekvalue(machine, immediate_key, value_num_name);
				auto [type3, size3] = reg::peekvalue(machine, shallow_key, value_num_name);
				auto [type4, size4] = reg::peekvalue(machine, deep_key, value_num_name);

				Assert::AreEqual(type1, REG_DWORD);
				Assert::AreEqual(type2, REG_DWORD);
				Assert::AreEqual(type3, REG_DWORD);
				Assert::AreEqual(type4, REG_DWORD);

				Assert::AreEqual(size1, sizeof(DWORD));
				Assert::AreEqual(size2, sizeof(DWORD));
				Assert::AreEqual(size3, sizeof(DWORD));
				Assert::AreEqual(size4, sizeof(DWORD));

				// cleanup
				reg::remove::value(machine, "", value_num_name);
				reg::remove::cluster(machine, IMMEDIATE_KEY_ROOT);
				reg::remove::cluster(machine, SHALLOW_KEY_ROOT);
				reg::remove::cluster(machine, DEEP_KEY_ROOT);
				});
		}

		TEST_METHOD(Value_Is_String)
		{
			std::string_view str1 = "";
			std::string_view str2 = "a";
			std::string_view str3 = "abc123";
			std::string_view str4 = "\n\t\r\r\nI Have a ball\r\r\r\t\t\t\n\n\n\n";

			test_with([str1, str2, str3, str4](HKEY machine) {
				reg::remove::value(machine, "", value_str_name);
				reg::remove::cluster(machine, IMMEDIATE_KEY_ROOT);
				reg::remove::cluster(machine, SHALLOW_KEY_ROOT);
				reg::remove::cluster(machine, DEEP_KEY_ROOT);

				reg::create::string(machine, "", value_str_name, str1);
				reg::create::string(machine, immediate_key, value_str_name, str2);
				reg::create::string(machine, shallow_key, value_str_name, str3);
				reg::create::string(machine, deep_key, value_str_name, str4);
				// -- setup

				auto [type1, size1] = reg::peekvalue(machine, "", value_str_name);
				auto [type2, size2] = reg::peekvalue(machine, immediate_key, value_str_name);
				auto [type3, size3] = reg::peekvalue(machine, shallow_key, value_str_name);
				auto [type4, size4] = reg::peekvalue(machine, deep_key, value_str_name);

				Assert::AreEqual(type1, REG_SZ);
				Assert::AreEqual(type2, REG_SZ);
				Assert::AreEqual(type3, REG_SZ);
				Assert::AreEqual(type4, REG_SZ);

				// +1 to account for null termination
				Assert::AreEqual(size1, str1.size() + 1);
				Assert::AreEqual(size2, str2.size() + 1);
				Assert::AreEqual(size3, str3.size() + 1);
				Assert::AreEqual(size4, str4.size() + 1);

				// cleanup
				reg::remove::value(machine, "", value_str_name);
				reg::remove::cluster(machine, IMMEDIATE_KEY_ROOT);
				reg::remove::cluster(machine, SHALLOW_KEY_ROOT);
				reg::remove::cluster(machine, DEEP_KEY_ROOT);
				});
		}
	};
}

namespace Create
{
	TEST_CLASS(Key)
	{
	public:
		TEST_CLASS_INITIALIZE(class_setup) {}
		TEST_CLASS_CLEANUP(class_cleanup) {}
		TEST_METHOD_INITIALIZE(method_setup) {}
		TEST_METHOD_CLEANUP(method_cleanup) {}

		TEST_METHOD(Create_Key_Under_Root)
		{
			test_with([](HKEY machine) {
				auto [handle, disposition] = reg::create::key(machine, "TestKey1");
				Assert::IsTrue(disposition == reg::create::Disposition::CREATED_KEY);

				// cleanup
				if (!reg::remove::cluster(machine, "TestKey1"))
					Assert::Fail();
				});
		}

		TEST_METHOD(Create_Nested_Key_Under_Root)
		{
			test_with([](HKEY machine) {
				auto [handle, disposition] = reg::create::key(machine, "TestKey2\\Subkey");
				Assert::IsTrue(disposition == reg::create::Disposition::CREATED_KEY);

				// cleanup
				if (!reg::remove::cluster(machine, "TestKey2"))
					Assert::Fail();
				});
		}
	};

	TEST_CLASS(Value)
	{
	public:

		TEST_CLASS_INITIALIZE(class_setup) {
			test_with([](HKEY machine) {
				reg::create::key(machine, immediate_key);
				reg::create::key(machine, shallow_key);
				reg::create::key(machine, deep_key);
				});
		}
		TEST_CLASS_CLEANUP(class_cleanup) {
			test_with([](HKEY machine) {
				reg::remove::cluster(machine, IMMEDIATE_KEY_ROOT);
				reg::remove::cluster(machine, SHALLOW_KEY_ROOT);
				reg::remove::cluster(machine, DEEP_KEY_ROOT);
				});
		}
		TEST_METHOD_INITIALIZE(method_setup) {}
		TEST_METHOD_CLEANUP(method_cleanup) {}

		TEST_METHOD(Create_num_Value_Under_Root)
		{
			test_with([](HKEY machine) {
				reg::remove::value(machine, "", value_str_name);
				// -- setup

				auto [handle, disposition] = reg::create::number(machine, immediate_key, value_num_name);
				Assert::IsTrue(disposition == reg::create::Disposition::CREATED_VALUE);

				// cleanup
				if (!reg::remove::value(machine, immediate_key, value_num_name))
					Assert::Fail();
				});
		}

		TEST_METHOD(Create_str_Value_Under_Root)
		{
			test_with([](HKEY machine) {
				reg::remove::value(machine, "", value_str_name);
				// -- setup

				auto [handle, disposition] = reg::create::string(machine, "", value_str_name);
				Assert::IsTrue(disposition == reg::create::Disposition::CREATED_VALUE);

				// cleanup
				if (!reg::remove::value(machine, "", value_str_name))
					Assert::Fail();
				});

		}

		TEST_METHOD(Create_num_Value_Under_Key)
		{
			test_with([](HKEY machine) {
				reg::remove::value(machine, immediate_key, value_num_name);
				reg::remove::value(machine, shallow_key, value_num_name);
				reg::remove::value(machine, deep_key, value_num_name);
				// -- setup

				auto [h1, d1] = reg::create::number(machine, immediate_key, value_num_name);
				Assert::IsTrue(d1 == reg::create::Disposition::CREATED_VALUE);
				auto [h2, d2] = reg::create::number(machine, shallow_key, value_num_name);
				Assert::IsTrue(d2 == reg::create::Disposition::CREATED_VALUE);
				auto [h3, d3] = reg::create::number(machine, deep_key, value_num_name);
				Assert::IsTrue(d3 == reg::create::Disposition::CREATED_VALUE);

				// cleanup
				if (!reg::remove::value(machine, immediate_key, value_num_name))
					Assert::Fail();
				if (!reg::remove::value(machine, shallow_key, value_num_name))
					Assert::Fail();
				if (!reg::remove::value(machine, deep_key, value_num_name))
					Assert::Fail();
				});
		}

		TEST_METHOD(Create_str_Value_Under_Key)
		{
			test_with([](HKEY machine) {
				reg::remove::value(machine, immediate_key, value_str_name);
				reg::remove::value(machine, shallow_key, value_str_name);
				reg::remove::value(machine, deep_key, value_str_name);
				// -- setup

				auto [h1, d1] = reg::create::string(machine, immediate_key, value_str_name);
				Assert::IsTrue(d1 == reg::create::Disposition::CREATED_VALUE);
				auto [h2, d2] = reg::create::string(machine, shallow_key, value_str_name);
				Assert::IsTrue(d2 == reg::create::Disposition::CREATED_VALUE);
				auto [h3, d3] = reg::create::string(machine, deep_key, value_str_name);
				Assert::IsTrue(d3 == reg::create::Disposition::CREATED_VALUE);

				// cleanup
				if (!reg::remove::value(machine, immediate_key, value_str_name))
					Assert::Fail();
				if (!reg::remove::value(machine, shallow_key, value_str_name))
					Assert::Fail();
				if (!reg::remove::value(machine, deep_key, value_str_name))
					Assert::Fail();
				});
		}
	};
}

namespace Remove
{
	TEST_CLASS(Key)
	{
	public:
		TEST_CLASS_INITIALIZE(class_setup) {}
		TEST_CLASS_CLEANUP(class_cleanup) {}
		TEST_METHOD_INITIALIZE(method_setup) {}
		TEST_METHOD_CLEANUP(method_cleanup) {}
		TEST_METHOD(Remove_Key_Under_Root)
		{
			test_with([](HKEY machine) {
				const char* const key_name = "TestKey1";

				reg::remove::cluster(machine, key_name);
				reg::create::key(machine, key_name);
				// -- setup

				if (!reg::remove::key(machine, key_name))
					Assert::Fail();
				});
		}

		TEST_METHOD(Remove_Subkey)
		{
			test_with([](HKEY machine) {
#ifdef KEY_BASE
#undef KEY_BASE
#endif // KEY_BASE
#define KEY_BASE "TestKey2"

				const char* const key_name = KEY_BASE "\\Subkey";

				reg::create::key(machine, key_name);
				// -- setup

				if (!reg::remove::key(machine, key_name))
					Assert::Fail();

				Assert::IsTrue(reg::key_exists(machine, KEY_BASE));

				// cleanup
				if (!reg::remove::key(machine, KEY_BASE))
					Assert::Fail();
				});

		}
	};

	TEST_CLASS(Value)
	{
		TEST_CLASS_INITIALIZE(class_setup) {
			test_with([](HKEY machine) {
				reg::create::key(machine, immediate_key);
				reg::create::key(machine, shallow_key);
				reg::create::key(machine, deep_key);
				});
		}
		TEST_CLASS_CLEANUP(class_cleanup) {
			test_with([](HKEY machine) {
				reg::remove::cluster(machine, IMMEDIATE_KEY_ROOT);
				reg::remove::cluster(machine, shallow_key);
				reg::remove::cluster(machine, deep_key);
				});
		}
		TEST_METHOD_INITIALIZE(method_setup) {}
		TEST_METHOD_CLEANUP(method_cleanup) {}

		TEST_METHOD(Remove_num_Value_Under_Root)
		{
			test_with([](HKEY machine) {
				reg::create::number(machine, "", value_num_name);
				// -- setup

				if (!reg::remove::value(machine, "", value_num_name))
					Assert::Fail();
				});
		}

		TEST_METHOD(Remove_str_Value_Under_Root)
		{
			test_with([](HKEY machine) {
				reg::create::string(machine, "", value_str_name);
				// -- setup

				if (!reg::remove::value(machine, "", value_str_name))
					Assert::Fail();
				});
		}

		TEST_METHOD(Remove_num_Value_Under_Key)
		{
			test_with([](HKEY machine) {
				reg::create::number(machine, immediate_key, value_num_name);
				reg::create::number(machine, shallow_key, value_num_name);
				reg::create::number(machine, deep_key, value_num_name);
				// -- setup

				if (!reg::remove::value(machine, immediate_key, value_num_name)) Assert::Fail();
				if (!reg::remove::value(machine, shallow_key, value_num_name)) Assert::Fail();
				if (!reg::remove::value(machine, deep_key, value_num_name)) Assert::Fail();
				});
		}

		TEST_METHOD(Remove_str_Value_Under_Key)
		{
			test_with([](HKEY machine) {
				reg::create::string(machine, immediate_key, value_str_name);
				reg::create::string(machine, shallow_key, value_str_name);
				reg::create::string(machine, deep_key, value_str_name);
				// -- setup

				if (!reg::remove::value(machine, immediate_key, value_str_name)) Assert::Fail();
				if (!reg::remove::value(machine, shallow_key, value_str_name)) Assert::Fail();
				if (!reg::remove::value(machine, deep_key, value_str_name)) Assert::Fail();
				});
		}
	};
}

namespace Query
{
	TEST_CLASS(Key)
	{
		TEST_CLASS_INITIALIZE(class_setup) {
			test_with([](HKEY machine) {
				reg::create::key(machine, immediate_key);
				reg::create::key(machine, shallow_key);
				reg::create::key(machine, deep_key);
				});
		}
		TEST_CLASS_CLEANUP(class_cleanup) {
			test_with([](HKEY machine) {
				reg::remove::cluster(machine, IMMEDIATE_KEY_ROOT);
				reg::remove::cluster(machine, SHALLOW_KEY_ROOT);
				reg::remove::cluster(machine, DEEP_KEY_ROOT);
				});
		}
		TEST_METHOD_INITIALIZE(method_setup) {}
		TEST_METHOD_CLEANUP(method_cleanup) {}

	public:

		TEST_METHOD(Number_Of_Keys_No_Nester_Subkeys)
		{
			const DWORD No_Keys_Immediate = 3;
			const DWORD No_Keys_Shallow = 10;
			const DWORD No_Keys_Deep = 200;

			test_with([No_Keys_Immediate, No_Keys_Shallow, No_Keys_Deep](HKEY machine) {
				createNKeys(machine, immediate_key, No_Keys_Immediate);
				createNKeys(machine, shallow_key, No_Keys_Shallow);
				createNKeys(machine, deep_key, No_Keys_Deep);
				// -- setup

				auto [subkeys1, _a1, _b1, _c1] = reg::query::key_info(machine, immediate_key);
				auto [subkeys2, _a2, _b2, _c2] = reg::query::key_info(machine, shallow_key);
				auto [subkeys3, _a3, _b3, _c3] = reg::query::key_info(machine, deep_key);

				Assert::AreEqual(subkeys1, No_Keys_Immediate);
				Assert::AreEqual(subkeys2, No_Keys_Shallow);
				Assert::AreEqual(subkeys3, No_Keys_Deep);

				// cleanup
				reg::remove::subkeys(machine, immediate_key);
				reg::remove::subkeys(machine, shallow_key);
				reg::remove::subkeys(machine, deep_key);
				});
		}

		TEST_METHOD(Number_Of_Values)
		{
			const DWORD No_Values_Immediate = 3;
			const DWORD No_Values_Shallow = 10;
			const DWORD No_Values_Deep = 200;

			test_with([No_Values_Immediate, No_Values_Shallow, No_Values_Deep](HKEY machine) {
				createNValues(machine, immediate_key, No_Values_Immediate);
				createNValues(machine, shallow_key, No_Values_Shallow);
				createNValues(machine, deep_key, No_Values_Deep);
				// -- setup

				auto [_a1, _b1, values1, _c1] = reg::query::key_info(machine, immediate_key);
				auto [_a2, _b2, values2, _c2] = reg::query::key_info(machine, shallow_key);
				auto [_a3, _b3, values3, _c3] = reg::query::key_info(machine, deep_key);

				Assert::AreEqual(values1, No_Values_Immediate);
				Assert::AreEqual(values2, No_Values_Shallow);
				Assert::AreEqual(values3, No_Values_Deep);

				// cleanup
				reg::remove::values(machine, immediate_key);
				reg::remove::values(machine, shallow_key);
				reg::remove::values(machine, deep_key);
				});
		}

		//TEST_METHOD(Key_Names)
		//TEST_METHOD(Value_Names);

	};

	TEST_CLASS(Value)
	{
	public:
		TEST_CLASS_INITIALIZE(class_setup) {
			test_with([](HKEY machine) {
				reg::create::key(machine, immediate_key);
				reg::create::key(machine, shallow_key);
				reg::create::key(machine, deep_key);
				});
		}
		TEST_CLASS_CLEANUP(class_cleanup) {
			test_with([](HKEY machine) {
				reg::remove::cluster(machine, IMMEDIATE_KEY_ROOT);
				reg::remove::cluster(machine, SHALLOW_KEY_ROOT);
				reg::remove::cluster(machine, DEEP_KEY_ROOT);
				});
		}
		TEST_METHOD_INITIALIZE(method_setup) {}
		TEST_METHOD_CLEANUP(method_cleanup) {}

		TEST_METHOD(Check_Number_Value) {
			const char* const immediate_value_name = "MyValue";
			const char* const shallow_value_name = "ShallowValue";
			const char* const deep_value_name = "val";

			const DWORD imm_val = 123;
			const DWORD sha_val = 456;
			const DWORD dee_val = 951;

			test_with([immediate_value_name, shallow_value_name, deep_value_name, imm_val, sha_val, dee_val](HKEY machine) {
				reg::create::number(machine, immediate_key, immediate_value_name, imm_val);
				reg::create::number(machine, shallow_key, shallow_value_name, sha_val);
				reg::create::number(machine, deep_key, deep_value_name, dee_val);
				// -- setup

				DWORD val1 = reg::query::number(machine, immediate_key, immediate_value_name);
				DWORD val2 = reg::query::number(machine, shallow_key, shallow_value_name);
				DWORD val3 = reg::query::number(machine, deep_key, deep_value_name);

				Assert::AreEqual(val1, imm_val);
				Assert::AreEqual(val2, sha_val);
				Assert::AreEqual(val3, dee_val);

				// cleanup
				reg::remove::value(machine, immediate_key, immediate_value_name);
				reg::remove::value(machine, shallow_key, shallow_value_name);
				reg::remove::value(machine, deep_key, deep_value_name);
				});
		}

		TEST_METHOD(Check_String_Value) {
			const char* const immediate_value_name = "StryngyVal";
			const char* const shallow_value_name = "AName";
			const char* const deep_value_name = "val";

			const char* const imm_val = "I have an apple";
			const char* const sha_val = "A aB bC cD dE eF fG gH hI iJ jK kL lM mN nO oP pQ qR rS sT tU uV vW wX xY yZ z1234567890";
			const char* const dee_val = "";

			test_with([immediate_value_name, shallow_value_name, deep_value_name, imm_val, sha_val, dee_val](HKEY machine) {
				reg::create::string(machine, immediate_key, immediate_value_name, imm_val);
				reg::create::string(machine, shallow_key, shallow_value_name, sha_val);
				reg::create::string(machine, deep_key, deep_value_name, dee_val);
				// -- setup

				std::string val1 = reg::query::string(machine, immediate_key, immediate_value_name);
				std::string val2 = reg::query::string(machine, shallow_key, shallow_value_name);
				std::string val3 = reg::query::string(machine, deep_key, deep_value_name);

				Assert::AreEqual(val1.c_str(), imm_val);
				Assert::AreEqual(val2.c_str(), sha_val);
				Assert::AreEqual(val3.c_str(), dee_val);

				// cleanup
				reg::remove::value(machine, immediate_key, immediate_value_name);
				reg::remove::value(machine, shallow_key, shallow_value_name);
				reg::remove::value(machine, deep_key, deep_value_name);
				});
		}
	};
}

namespace Update
{
	TEST_CLASS(Value)
	{
		TEST_CLASS_INITIALIZE(class_setup) {
			test_with([](HKEY machine) {
				reg::create::key(machine, immediate_key);
				reg::create::key(machine, shallow_key);
				reg::create::key(machine, deep_key);
				});
		}
		TEST_CLASS_CLEANUP(class_cleanup) {
			test_with([](HKEY machine) {
				reg::remove::cluster(machine, IMMEDIATE_KEY_ROOT);
				reg::remove::cluster(machine, SHALLOW_KEY_ROOT);
				reg::remove::cluster(machine, DEEP_KEY_ROOT);
				});
		}
		TEST_METHOD_INITIALIZE(method_setup) {}
		TEST_METHOD_CLEANUP(method_cleanup) {}
	public:

		TEST_METHOD(Update_Number_Value)
		{
			const char* const immediate_value_name = "MyValue";
			const char* const shallow_value_name = "ShallowValue";
			const char* const deep_value_name = "val";

			const DWORD imm_val = 123;
			const DWORD sha_val = 456;
			const DWORD dee_val = 951;

			const DWORD imm_new_val = 111;
			const DWORD sha_new_val = 0;
			const DWORD dee_new_val = MAXINT;

			test_with([immediate_value_name, shallow_value_name,
				deep_value_name, imm_val, sha_val, dee_val,
				imm_new_val, sha_new_val, dee_new_val](HKEY machine) {
					reg::create::number(machine, immediate_key, immediate_value_name, imm_val);
					reg::create::number(machine, shallow_key, shallow_value_name, sha_val);
					reg::create::number(machine, deep_key, deep_value_name, dee_val);
					// -- setup

					reg::update::number(machine, immediate_key, immediate_value_name, imm_new_val);
					reg::update::number(machine, shallow_key, shallow_value_name, sha_new_val);
					reg::update::number(machine, deep_key, deep_value_name, dee_new_val);

					Assert::AreEqual(reg::query::number(machine, immediate_key, immediate_value_name), imm_new_val);
					Assert::AreEqual(reg::query::number(machine, shallow_key, shallow_value_name), sha_new_val);
					Assert::AreEqual(reg::query::number(machine, deep_key, deep_value_name), dee_new_val);

					// cleanup
					reg::remove::value(machine, immediate_key, immediate_value_name);
					reg::remove::value(machine, shallow_key, shallow_value_name);
					reg::remove::value(machine, deep_key, deep_value_name);

				});
		}

		TEST_METHOD(Update_String_Value)
		{
			const char* const immediate_value_name = "MyValue";
			const char* const shallow_value_name = "ShallowValue";
			const char* const deep_value_name = "val";

			const char* const imm_val = "123";
			const char* const sha_val = "456";
			const char* const dee_val = "951";

			const char* const imm_new_val = "";
			const char* const sha_new_val = "999";
			const char* const dee_new_val = "abc";

			test_with([immediate_value_name, shallow_value_name,
				deep_value_name, imm_val, sha_val, dee_val,
				imm_new_val, sha_new_val, dee_new_val](HKEY machine) {
					reg::create::string(machine, immediate_key, immediate_value_name, imm_val);
					reg::create::string(machine, shallow_key, shallow_value_name, sha_val);
					reg::create::string(machine, deep_key, deep_value_name, dee_val);
					// -- setup

					reg::update::string(machine, immediate_key, immediate_value_name, imm_new_val);
					reg::update::string(machine, shallow_key, shallow_value_name, sha_new_val);
					reg::update::string(machine, deep_key, deep_value_name, dee_new_val);

					std::string val1 = reg::query::string(machine, immediate_key, immediate_value_name);
					std::string val2 = reg::query::string(machine, shallow_key, shallow_value_name);
					std::string val3 = reg::query::string(machine, deep_key, deep_value_name);


					Assert::AreEqual(val1.c_str(), imm_new_val);
					Assert::AreEqual(val2.c_str(), sha_new_val);
					Assert::AreEqual(val3.c_str(), dee_new_val);

					// cleanup
					reg::remove::value(machine, immediate_key, immediate_value_name);
					reg::remove::value(machine, shallow_key, shallow_value_name);
					reg::remove::value(machine, deep_key, deep_value_name);

				});
		}
	};
}