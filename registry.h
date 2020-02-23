#pragma once
#include <memory>
#include <stdexcept>
#include <tuple>
#include <functional>
#include <unordered_map>
#include <string_view>
#include <string>
#include <sstream>
#include <Windows.h>

namespace reg
{
	namespace {

		/// <summary>Converts a locale name (e.g. "en-US") to a language id (LCID)</summary>
		/// <param name='locale_name'>A wide string specifying the locale name.</param>
		/// <returns>The language id associated to the given locale</returns>
		DWORD locale_to_id(const wchar_t* locale_name = LOCALE_NAME_SYSTEM_DEFAULT)
		{
			int count = GetLocaleInfoEx(LOCALE_NAME_SYSTEM_DEFAULT, LOCALE_SNAME, NULL, NULL);
			std::unique_ptr<wchar_t[]> buffer = std::make_unique<wchar_t[]>(count);

			count = GetLocaleInfoEx(LOCALE_NAME_SYSTEM_DEFAULT, LOCALE_ILANGUAGE, buffer.get(), count);
			std::wstringstream ss;
			ss << buffer;

			int code = NULL;
			ss >> std::hex >> code;
			return code;
		}

		/// <summary>Converts an error code to a human readable message</summary>
		/// <param name='error_code'>A system error code<para/>
		/// (e.g. 0x0h ERROR_SUCCESS or 0x2h ERROR_FILE_NOT_FOUND)</param>
		std::string ErrorCodeToString(DWORD error_code)
		{
			LPTSTR message_buffer = nullptr;
			DWORD message_length = 0;

			message_length = FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				error_code,
				locale_to_id(),
				(LPTSTR)&message_buffer,
				0,
				NULL);

			std::string message;
			if (message_length)
			{
				message.assign(message_buffer, message_length);
				LocalFree(message_buffer);
			}
			else
				message = "Could not format message";

			return message;
		}

		const std::unordered_map<HKEY, std::string_view> str_hkey
		{
			{HKEY_CLASSES_ROOT,		"HKEY_CLASSES_ROOT"		},
			{HKEY_CURRENT_USER,		"HKEY_CURRENT_USER"		},
			{HKEY_LOCAL_MACHINE,	"HKEY_LOCAL_MACHINE"	},
			{HKEY_USERS,			"HKEY_USERS"			},
			{HKEY_CURRENT_CONFIG,	"HKEY_CURRENT_CONFIG"	},
		};

		const std::unordered_map<DWORD, std::string_view> str_type
		{
			{REG_NONE,							"No type"									},
			{REG_SZ,							"Nul terminated string"						},
			{REG_EXPAND_SZ,						"Nul terminated string"						},
			{REG_BINARY,						"Free form binary"							},
			{REG_DWORD,							"32-bit number"								},
			{REG_DWORD_BIG_ENDIAN,				"32-bit number"								},
			{REG_LINK,							"Symbolic Link"								},
			{REG_MULTI_SZ,						"Multiple Unicode strings"					},
			{REG_RESOURCE_LIST,					"Resource list in the resource map"			},
			{REG_FULL_RESOURCE_DESCRIPTOR,		"Resource list in the hardware description"	},
			{REG_RESOURCE_REQUIREMENTS_LIST,	""											},
			{REG_QWORD,							"64-bit number"								},
		};
	}

	namespace except
	{
		inline std::string toString(HKEY machine, std::string_view key = "", std::string_view value = "") {
			std::stringstream builder;

			builder << reg::str_hkey.at(machine) << "\\";

			if (!key.empty())
				builder << key << "\\";

			if (!value.empty())
				builder << value << "\\";

			return builder.str();
		}

		template<typename... Ts>
		constexpr std::string concat_string(Ts const& ... ts) {
			std::stringstream s;
			(s << ... << ts);
			return s.str();
		}

		class not_implemented : public std::logic_error {
		public:
			not_implemented() : std::logic_error("Function not yet implemented") {}
		};

		class not_found : public std::exception {
		public:
			not_found(std::string_view message) : exception(message.data()) {}
		};

		class key_not_found : public not_found {
		public:
			key_not_found(HKEY machine, std::string_view key)
				: not_found(reg::except::concat_string(
					"The key \"",
					reg::except::toString(machine, key),
					"\" does not exist"))
			{}
		};

		class value_not_found : public not_found {
		public:
			value_not_found(HKEY machine, std::string_view key, std::string_view value)
				: not_found(reg::except::concat_string(
					"The value \"",
					reg::except::toString(machine, key, value),
					"\" does not exist"))
			{}

			value_not_found(HKEY handle, std::string_view value)
				: not_found(reg::except::concat_string(
					"The handle does not contain value: \"",
					value,
					"\""))
			{}
		};

		class type_error : public std::exception {
		public:
			type_error(HKEY machine, std::string_view key, std::string_view value,
				std::string_view expected_type, std::string_view provided_type)
				: exception(reg::except::concat_string(
					"Error working with \"",
					toString(machine, key, value),
					"\" - expected a ", expected_type,
					", but found a ", provided_type
				).c_str())
			{}
		};
	}

	namespace assert {
		inline void success(DWORD error_code) {
			if (error_code != ERROR_SUCCESS)
				throw std::runtime_error(reg::ErrorCodeToString(error_code));
		}

		inline void equal(DWORD error_code, DWORD sys_code) {
			if (error_code != sys_code)
				throw std::runtime_error(reg::ErrorCodeToString(error_code));
		}
	}

	template<typename T>
	using deleted_unique_ptr = std::unique_ptr<T, std::function<void(T*)>>;

	/// <summary>Creates a unique_ptr to a HKEY
	/// that will close the registry key upon deletion.</summary>
	[[nodiscard]]
	inline deleted_unique_ptr<HKEY> self_closing_handle() {
		return deleted_unique_ptr<HKEY>(new HKEY, [](HKEY* handle) {RegCloseKey(*handle); });
	}

	/// <summary>Creates a unique_ptr from the given HKEY
	/// that will close the registry key upon deletion.</summary>
	/// <param name='handle'>A handle to an open registry key as returned by RegCreateKey(...) or RegOpenKey(...)</param>
	[[nodiscard]]
	inline deleted_unique_ptr<HKEY> self_closing_handle(HKEY* handle) {
		return deleted_unique_ptr<HKEY>(handle, [](HKEY* h) {RegCloseKey(*h); });
	}

	/// <summary>Opens a registry key with the desired access rights (e.g. KEY_READ or KEY_WRITE)
	/// and returns a handle to that key.<para/>
	/// Throws an exception if the key cannot be opened or created</summary>
	/// <param name='machine'>Root key in the hierarchy</param>
	/// <param name='key'>Subkey to the desired node</param>
	/// <param name='rights'>A mask that specifies the desired access rights to the key</param>
	[[nodiscard]]
	inline PHKEY open(HKEY machine, std::string_view key, REGSAM rights)
	{
		PHKEY handle = new HKEY;
		DWORD result = NULL;

		// RegOpenKeyEx(HKEY hkey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult)
		// hkey			- main hierarchical key
		// lpSubKey		- subkey in the tree
		// ulOptions	- option to open key as a symbolic link
		// samDesired	- desired access rights
		// phkResult	- handle to the opened key
		result = RegOpenKeyEx(machine, key.data(), NULL, rights, handle);

		reg::assert::success(result);

		return handle;
	}

	/// <summary>Checks whether a given key exists or not in the windows registry</summary>
	/// <param name='machine'>Root key in the hierarchy</param>
	/// <param name='key'>Name of the subkey to be checked</param>
	/// <returns>The function returns true if the key exists at the specified path,
	/// otherwise it returns false.</returns>+
	[[nodiscard]]
	inline bool key_exists(HKEY machine, std::string_view key) noexcept
	{
		auto handle = self_closing_handle();

		DWORD result = NULL;
		result = RegOpenKeyEx(machine, key.data(), NULL, KEY_QUERY_VALUE, handle.get());

		return result == ERROR_SUCCESS;
	}

	/// <summary>Checks whether a given value exists or not in the windows registry</summary>
	/// <param name='machine'>Root key in the hierarchy</param>
	/// <param name='key'>Subkey to the desired node</param>
	/// <param name='value'>Name of the value to be checked</param>
	/// <returns>The function returns true if the specified value exists,
	/// otherwise it returns false</returns>
	[[nodiscard]]
	inline bool value_exists(HKEY machine, std::string_view key, std::string_view value) noexcept
	{
		auto handle = self_closing_handle();

		DWORD result = NULL;
		result = RegOpenKeyEx(machine, key.data(), NULL, KEY_QUERY_VALUE, handle.get());
		if (result == ERROR_SUCCESS)
		{
			// RegQueryValueEx(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
			// hKey			- handle to an open key
			// lpValueName	- name of the value to query
			// lpReserved	- must be NULL
			// lpType		- type of data associated with the specified value (can be NULL)
			// lpData		- buffer that receives the values data (can be NULL)
			// lpcbData		- variable that specifies the size, in bytes, of the buffer
			result = RegQueryValueEx(*handle, value.data(), NULL, NULL, NULL, NULL);
			return result == ERROR_SUCCESS;
		}
		else
			return false;
	}

	/// <summary>Checks whether a given value exists or not in the windows registry</summary>
	/// <param name='handle'>Handle to an open registry key</param>
	/// <param name='value'>Name of the value to be checked</param>
	/// <returns>The function returns true if the specified value exists,
	/// otherwise it returns false</returns>
	[[nodiscard]]
	inline bool value_exists(HKEY handle, std::string_view value) noexcept
	{
		DWORD result = NULL;
		result = RegQueryValueEx(handle, value.data(), NULL, NULL, NULL, NULL);
		return result == ERROR_SUCCESS;
	}

	namespace {
		/// <summary>Throws an exception if the key does not exist</summary>
		/// <param name='machine'>Root key in the hierarchy</param>
		/// <param name='key'>Subkey to the desired node</param>
		inline void _check_key(HKEY machine, std::string_view key)
		{
			bool ok = key_exists(machine, key);
			if (!ok)
				throw reg::except::key_not_found(machine, key);
		}

		/// <summary>Throws an exception if the value does not exist</summary>
		/// <param name='machine'>Root key in the hierarchy</param>
		/// <param name='key'>Subkey to the desired node</param>
		/// <param name='value'>Name of the value to be checked</param>
		inline void _check_value(HKEY machine, std::string_view key, std::string_view value)
		{
			bool ok = value_exists(machine, key, value);
			if (!ok)
				throw reg::except::value_not_found(machine, key, value);
		}

		/// <summary>Throws an exception if the value does not exist</summary>
		/// <param name='machine'>Root key in the hierarchy</param>
		/// <param name='key'>Subkey to the desired node</param>
		/// <param name='value'>Name of the value to be checked</param>
		inline void _check_value(HKEY handle, std::string_view value)
		{
			bool ok = value_exists(handle, value);
			if (!ok)
				throw reg::except::value_not_found(handle, value);
		}
	}

	/// <summary>Retrieves the type and size of the registry value.<para/>
	/// Throws an exception if
	/// the key does not exist
	/// or the value does not exist.<para/>
	/// To check if a key exists, use <see cref="key_exists"/><para/>
	/// To check if a value exists, use <see cref="value_exists"/></summary>
	/// <param name='machine'>Root key in the hierarchy</param>
	/// <param name='key'>Subkey to the desired node</param>
	/// <param name='value'>Name of the value to be queried</param>
	[[nodiscard]]
	inline std::tuple<DWORD, size_t> peekvalue(HKEY machine, std::string_view key, std::string_view value)
	{
		reg::_check_key(machine, key);
		reg::_check_value(machine, key, value);

		DWORD type = -1;
		DWORD size = -1;
		DWORD result = NULL;

		// RegGetValueA(HKEY hkey, LPCSTR lpSubKey, LPCSTR lpValue, DWORD dwFlags, LPDWORD pdwType, PVOID pvData, LPDWORD pcbData)
		// hkey		- main hierarchical key
		// lpSubKey	- subkey in the tree
		// lpValue	- name of the value to query
		// dwFlags	- type restriction (RRF_RT_ANY = No type restriction)
		// pdwType	- variable that receives a code indicating the type of data stored in the specified value
		// pvData	- buffer that receives the value's data (can be NULL)
		// pcbData	- variable that specifies the size of the buffer
		result = RegGetValue(machine, key.data(), value.data(), RRF_RT_ANY, &type, NULL, &size);
		reg::assert::success(result);

		// probably a bug in RegGetValue.
		// Registry uses wchars to store strings. Normal characters are counted
		// correctly, but the null termination is counted twice
		if (type == REG_SZ)
			--size;

		return { type, size };
	}

	/// <summary>Retrieves the type and size of the registry value.<para/>
	/// Throws an exception if
	/// the value does not exist.<para/>
	/// To check if a value exists, use <see cref="value_exists"/></summary>
	/// <param name='handle'>Handle to an open registry key</param>
	/// <param name='value'>Name of the value to be queried</param>
	[[nodiscard]]
	inline std::tuple<DWORD, size_t> peekvalue(HKEY handle, std::string_view value)
	{
		reg::_check_value(handle, value);

		DWORD type = -1;
		DWORD size = -1;
		DWORD result = NULL;
		result = RegGetValue(handle, "", value.data(), RRF_RT_ANY, &type, NULL, &size);

		reg::assert::success(result);

		return { type, size };
	}

	namespace
	{
		/// <summary>Throws an exception if the value's type is not the one provided.<para/>
		/// Additionally, this function also throws an exception if the key or value are not found</summary>
		/// <param name='machine'>Root key in the hierarchy</param>
		/// <param name='key'>Subkey to the desired node</param>
		/// <param name='value'>Name of the value to be checked</param>
		/// <param name='type'>One of the Registry Value Types (e.g. REG_DWORD, REG_QWORD, REG_SZ, REG_NONE)</param>
		inline void _check_type(HKEY machine, std::string_view key, std::string_view value, DWORD type)
		{
			reg::_check_key(machine, key);
			reg::_check_value(machine, key, value);

			auto [mytype, size] = peekvalue(machine, key, value);
			if (mytype != type)
				throw reg::except::type_error(machine, key, value, str_type.at(type), str_type.at(mytype));
		}
	}

	namespace security
	{
		/// <summary>Retrieves the <see cref="SECURITY_DESCRIPTOR"/> protecting the specified
		/// open registry key. The handle must have been opened with READ_CONTROL access.<para/>
		/// The <see cref="SECURITY_DESCRIPTOR"/> must be deallocated by the caller.</summary>
		/// <param name='handle'>Handle to an open registry key</param>
		/// <returns>A pointer to a block of memory containing a <see cref="SECURITY_DESCRIPTOR"/></returns>
		inline SECURITY_DESCRIPTOR* get_security_descriptor(HKEY handle)
		{
			DWORD code = NULL;
			DWORD size = 0;
			// Query how much space we need to allocate for the security descriptor
			code = RegGetKeySecurity(
				handle,
				DACL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION,
				NULL,
				&size);

			reg::assert::equal(code, ERROR_INSUFFICIENT_BUFFER);

			// Allocate the space required to hold the structure
			SECURITY_DESCRIPTOR* buff = reinterpret_cast<SECURITY_DESCRIPTOR*>(operator new(size));

			// Fill the structure
			code = RegGetKeySecurity(
				handle,
				DACL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION,
				buff,
				&size);

			reg::assert::success(code);

			return buff;
		}

		/// <summary>Retrieves the <see cref="SECURITY_DESCRIPTOR"/> protecting the specified
		/// registry key.<para/>
		/// The <see cref="SECURITY_DESCRIPTOR"/> must be deallocated by the caller.</summary>
		/// <param name='machine'>Root key in the hierarchy</param>
		/// <param name='key'>Subkey to the desired node</param>
		/// <returns>A pointer to a block of memory containing a <see cref="SECURITY_DESCRIPTOR"/></returns>
		inline SECURITY_DESCRIPTOR* get_security_descriptor(HKEY machine, std::string_view key)
		{
			_check_key(machine, key);

			auto handle = reg::self_closing_handle(reg::open(machine, key, READ_CONTROL));
			return reg::security::get_security_descriptor(*handle);
		}

		/// <summary>Changes permissions for the specified key.</summary>
		inline void gain_permission(HKEY machine, std::string_view key) {
			reg::_check_key(machine, key);

			auto handle = reg::self_closing_handle(reg::open(machine, key, READ_CONTROL));

			std::unique_ptr<SECURITY_DESCRIPTOR> descriptor(reg::security::get_security_descriptor(*handle));

			PSID owner = nullptr;
			BOOL owner_defaulted;

			PACL dacl = nullptr;
			BOOL dacl_present;
			BOOL dacl_defaulted;

			GetSecurityDescriptorOwner(descriptor.get(), &owner, &owner_defaulted);
			GetSecurityDescriptorDacl(descriptor.get(), &dacl_present, &dacl, &dacl_defaulted);
		}
	}

	namespace query
	{
		namespace {
			/// <summary>Not implemented - throws an exception</summary>
			template<typename T>
			T _get_data(HKEY, std::string_view, std::string_view) {
				throw reg::except::not_implemented();
			}

			/// <summary>Retrieves a DWORD from the specified registry value<para/>
			/// Throws an exception if it fails</summary>
			/// <param name='machine'>Root key in the hierarchy</param>
			/// <param name='key'>Subkey to the desired node</param>
			/// <param name='value'>Name of the value to be queried</param>
			/// <returns>The data found in the registry value</returns>
			template<>
			DWORD _get_data(HKEY machine, std::string_view key, std::string_view value)
			{
				auto handle = self_closing_handle(open(machine, key, KEY_QUERY_VALUE));

				DWORD code = NULL;
				DWORD data = NULL;
				DWORD buff_size = sizeof(DWORD);
				code = RegGetValue(machine, key.data(), value.data(), RRF_RT_REG_DWORD, NULL, &data, &buff_size);

				reg::assert::success(code);

				return data;
			}

			/// <summary>Retrieves a string from the specified registry value<para/>
			/// Throws an exception if it fails</summary>
			/// <param name='machine'>Root key in the hierarchy</param>
			/// <param name='key'>Subkey to the desired node</param>
			/// <param name='value'>Name of the value to be queried</param>
			/// <returns>The data found in the registry value</returns>
			template<>
			std::string _get_data(HKEY machine, std::string_view key, std::string_view value)
			{
				auto handle = self_closing_handle(open(machine, key, KEY_QUERY_VALUE));

				// get the size of the string to be able to allocate
				auto [type, size] = peekvalue(machine, key, value);

				std::string data(size, '\0');
				DWORD code = NULL;
				DWORD buff_size = static_cast<DWORD>(size);
				code = RegGetValue(machine, key.data(), value.data(), RRF_RT_REG_SZ, NULL, data.data(), &buff_size);

				reg::assert::success(code);

				return data;
			}
		}

		/// <summary>Retrieves a number from the specified registry value.<para/>
		/// Throws an exception if
		/// the key does not exist,
		/// the value does not exist,
		/// the value is not a number
		/// or the function fails to retrieve the data</summary>
		/// <param name='machine'>Root key in the hierarchy</param>
		/// <param name='key'>Subkey to the desired node</param>
		/// <param name='value'>Name of the value to be queried</param>
		/// <returns>The data found in the registry value</returns>
		inline DWORD number(HKEY machine, std::string_view key, std::string_view value)
		{
			reg::_check_type(machine, key, value, REG_DWORD);

			DWORD result = _get_data<DWORD>(machine, key, value);
			return result;
		}

		/// <summary>Retrieves a string from the specified registry value.<para/>
		/// Throws an exception if
		/// the key does not exist,
		/// the value does not exist,
		/// the value is not a string
		/// or the function fails to retrieve the data</summary>
		/// <param name='machine'>Root key in the hierarchy</param>
		/// <param name='key'>Subkey to the desired node</param>
		/// <param name='value'>Name of the value to be queried</param>
		/// <returns>The data found in the registry value</returns>
		inline std::string string(HKEY machine, std::string_view key, std::string_view value)
		{
			reg::_check_type(machine, key, value, REG_SZ);

			std::string result = _get_data<std::string>(machine, key, value);
			return result;
		}

		/// <summary>For a given handle to an open registry key, retrieves in this order:<para/>
		/// - the number of subkeys<para/>
		/// - the length of the longest subkey (null termination included)<para/>
		/// - the number of values the key contains<para/>
		/// - the length of the longest value (null termination included)<para/>
		/// Throws an exception if the key cannot be queried.</summary>
		/// <param name='handle'>Handle to an open registry key.<para/>
		/// The key must have been opened with the KEY_ENUMERATE_SUB_KEYS access right.</param>
		/// <returns>The number of subkeys,
		/// length of the longest subkey,
		/// the number of values and
		/// length of the longest value</returns>
		inline std::tuple<DWORD, DWORD, DWORD, DWORD> key_info(HKEY handle)
		{
			DWORD subkeys = 0;
			DWORD subvalues = 0;
			DWORD maxkeynamelen = 0;
			DWORD maxvaluenamelen = 0;

			DWORD code = NULL;
			code = RegQueryInfoKey(
				handle,
				NULL,
				NULL,
				NULL,
				&subkeys,
				&maxkeynamelen,
				NULL,
				&subvalues,
				&maxvaluenamelen,
				NULL,
				NULL,
				NULL
			);

			reg::assert::success(code);

			return { subkeys, maxkeynamelen + 1, subvalues, maxvaluenamelen + 1 };
		}

		/// <summary>For an arbitrary registry key, retrieves in this order:<para/>
		/// - the number of subkeys<para/>
		/// - the length of the longest subkey (null termination included)<para/>
		/// - the number of values the key contains<para/>
		/// - the length of the longest value (null termination included)<para/>
		/// Throws an exception if the key cannot be queried.</summary>
		/// <param name='machine'>Root key in the hierarchy</param>
		/// <param name='key'>Subkey to the desired node</param>
		/// <returns>The number of subkeys,
		/// length of the longest subkey,
		/// the number of values and
		/// length of the longest value</returns>
		inline std::tuple<DWORD, DWORD, DWORD, DWORD> key_info(HKEY machine, std::string_view key)
		{
			reg::_check_key(machine, key);

			auto handle = reg::self_closing_handle(reg::open(machine, key, KEY_QUERY_VALUE));
			return reg::query::key_info(*handle);
		}

		/// <summary>For a given handle to an open registry key, retrieves the names of all its subkeys.</summary>
		/// <param name='handle'>Handle to an open registry key.<para/>
		/// The key must have been opened with the KEY_ENUMERATE_SUB_KEYS access right.</param>
		inline std::vector<std::string> keys(HKEY handle)
		{
			std::vector<std::string> enum_keys;

			const auto [subkeys, maxkeynamelen, subvalues, maxvaluenamelen] = reg::query::key_info(handle);
			enum_keys.reserve(subkeys);
			std::unique_ptr<char[]> buffer = std::make_unique<char[]>(maxkeynamelen);

			DWORD i = 0;
			DWORD code = NULL;
			do
			{
				DWORD characters_read = maxkeynamelen;
				code = RegEnumKeyEx(handle, i++, buffer.get(), &characters_read, NULL, NULL, NULL, NULL);

				if (code == ERROR_NO_MORE_ITEMS)
					break;

				enum_keys.emplace_back(buffer.get(), characters_read);

			} while (code == ERROR_SUCCESS);

			reg::assert::equal(code, ERROR_NO_MORE_ITEMS);

			return enum_keys;
		}

		/// <summary>For an arbitrary registry key, retrieves the names of all its subkeys.</summary>
		/// <param name='machine'>Root key in the hierarchy</param>
		/// <param name='key'>Subkey to the desired node</param>
		/// <returns>A vector containing the name of every subkey found.</returns>
		inline std::vector<std::string> keys(HKEY machine, std::string_view key)
		{
			reg::_check_key(machine, key);

			auto handle = self_closing_handle(reg::open(machine, key, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS));
			return reg::query::keys(*handle);
		}

		/// <summary>For a given handle to an open registry key, retrieves all of its underlying values.</summary>
		/// <param name='handle'>Handle to an open registry key.</param>
		/// <returns>A vector containing the name of every value found.</returns>
		inline std::vector<std::string> value_names(HKEY handle)
		{
			std::vector<std::string> enum_values;

			const auto [subkeys, maxkeynamelen, subvalues, maxvaluenamelen] = reg::query::key_info(handle);
			enum_values.reserve(subvalues);
			std::unique_ptr<char[]> buffer = std::make_unique<char[]>(maxvaluenamelen);

			DWORD i = 0;
			DWORD code = NULL;
			do
			{
				DWORD characters_read = maxvaluenamelen;
				code = RegEnumValue(handle, i++, buffer.get(), &characters_read, NULL, NULL, NULL, NULL);

				if (code == ERROR_NO_MORE_ITEMS)
					break;

				enum_values.emplace_back(buffer.get(), characters_read);

			} while (code == ERROR_SUCCESS);

			reg::assert::equal(code, ERROR_NO_MORE_ITEMS);

			return enum_values;
		}

		/// <summary>For an arbitrary registry key, retrieves all of its underlying values.</summary>
		/// <param name='machine'>Root key in the hierarchy</param>
		/// <param name='key'>Subkey to the desired node</param>
		/// <returns>A vector containing the name of every value found.</returns>
		inline std::vector<std::string> value_names(HKEY machine, std::string_view key)
		{
			reg::_check_key(machine, key);

			auto handle = self_closing_handle(reg::open(machine, key, KEY_QUERY_VALUE));
			return value_names(*handle);
		}
	}

	namespace update
	{
		namespace {
			/// <summary>Sets the data of the specified DWORD value under the given registry key.<para/>
			/// Throws an exception if it fails.</summary>
			/// <param name='handle'>Handle to an open registry key</param>
			/// <param name='value'>Name of the value to be modified</param>
			/// <param name='data'>The new value</param>
			void _set_data(HKEY handle, std::string_view value, DWORD data)
			{
				DWORD code = NULL;
				code = RegSetValueEx(handle, value.data(), NULL, REG_DWORD, (const BYTE*)&data, sizeof(data));

				reg::assert::success(code);
			}

			/// <summary>Sets the data of a specified DWORD value under a registry key.<para/>
			/// Throws an exception if it fails.</summary>
			/// <param name='machine'>Root key in the hierarchy</param>
			/// <param name='key'>Subkey to the desired node</param>
			/// <param name='value'>Name of the value to be modified</param>
			/// <param name='data'>The new value</param>
			void _set_data(HKEY machine, std::string_view key, std::string_view value, DWORD data)
			{
				auto handle = self_closing_handle(reg::open(machine, key, KEY_SET_VALUE));
				reg::update::_set_data(*handle, value, data);
			}

			/// <summary>Sets the data of a specified string under a registry key.</para>
			/// Throws an exception if it fails</summary>
			/// <param name='handle'>Handle to an open registry key</param>
			/// <param name='value'>Name of the value to be modified</param>
			/// <param name='data'>The new value</param>
			void _set_data(HKEY handle, std::string_view value, std::string_view data)
			{
				DWORD code = NULL;
				const DWORD data_size = static_cast<DWORD>(data.size() + 1); // +1 to account for null ending
				const BYTE* data_ptr = reinterpret_cast<const BYTE*>(data.data());
				code = RegSetValueEx(handle, value.data(), NULL, REG_SZ, data_ptr, data_size);

				reg::assert::success(code);
			}

			/// <summary>Sets the data of a specified DWORD value under a registry key.<para/>
			/// Throws an exception if it fails.</summary>
			/// <param name='machine'>Root key in the hierarchy</param>
			/// <param name='key'>Subkey to the desired node</param>
			/// <param name='value'>Name of the value to be modified</param>
			/// <param name='data'>The new value</param>
			void _set_data(HKEY machine, std::string_view key, std::string_view value, std::string_view data)
			{
				auto handle = self_closing_handle(reg::open(machine, key, KEY_SET_VALUE));
				reg::update::_set_data(*handle, value, data);
			}
		}

		/// <summary>Sets the data of a specified DWORD value under a registry key.<para/>
		/// Throws an exception if
		/// the key does not exist,
		/// the value does not exist,
		/// the value is not a DWORD
		/// or the function fails to set the new data</summary>
		/// <param name='machine'>Root key in the hierarchy</param>
		/// <param name='key'>Subkey to the desired node</param>
		/// <param name='value'>Name of the value to be modified</param>
		/// <param name='data'>The new value</param>
		inline void number(HKEY machine, std::string_view key, std::string_view value, DWORD data)
		{
			reg::_check_type(machine, key, value, REG_DWORD);

			reg::update::_set_data(machine, key, value, data);
		}

		/// <summary>Sets the data of a specified string value under a registry key.<para/>
		/// Throws an exception if
		/// the key does not exist,
		/// the value does not exist,
		/// the value is not a string,
		/// or the function fails to set the new data</summary>
		/// <param name='machine'>Root key in the hierarchy</param>
		/// <param name='key'>Subkey to the desired node</param>
		/// <param name='value'>Name of the value to be modified</param>
		/// <param name='data'>The new value</param>
		inline void string(HKEY machine, std::string_view key, std::string_view value, std::string_view data)
		{
			reg::_check_type(machine, key, value, REG_SZ);

			reg::update::_set_data(machine, key, value, data);
		}
	}

	namespace create {
		namespace {
			enum class Disposition {
				UNKNOWN = NULL,
				CREATED_KEY = REG_CREATED_NEW_KEY,
				EXISTS_KEY = REG_OPENED_EXISTING_KEY,
				CREATED_VALUE,
				EXISTS_VALUE
			};

			/// <summary>Creates the specified registry key.
			/// If the key already exists, the function opens it.<para/>
			/// Throws an exception if it fails.</summary>
			/// <param name='machine'>Root key in the hierarchy</param>
			/// <param name='key'>Subkey to the desired node</param>
			/// <returns>A tuple containing a handle to the opened key and
			/// a value that can only be REG_CREATED_NEW_KEY, REG_OPENED_EXISTING_KEY or NULL</returns>
			std::tuple<PHKEY, Disposition> _create_key(HKEY machine, std::string_view key)
			{
				PHKEY handle = nullptr;
				DWORD disposition = NULL;

				if (reg::key_exists(machine, key))
				{
					disposition = REG_OPENED_EXISTING_KEY;
					handle = reg::open(machine, key, KEY_READ | KEY_WRITE);
				}
				else
				{
					handle = new HKEY;
					DWORD result = NULL;
					// RegCreateKeyEx(
					//		HKEY hKey									- main hierarchical key
					//		LPCWSTR lpSubKey							- subkey in the tree
					//		DWORD Reserved								- must be NULL
					//		LPWSTR lpClass								- user-defined class type of this key (can be NULL)
					//		DWORD dwOptions								- type of key
					//		REGSAM samDesired							- mask that specifies the access rights for the key
					//		LPSECURITY_ATTRIBUTES lpSecurityAttributes	- a SECURITY_ATTRIBUTES structure (can be NULL)
					//		PHKEY phkResult								- handle to the opened or created key
					//		LPDWORD lpdwDisposition						- variable that receives disposition value
					// )
					result = RegCreateKeyEx(
						machine, key.data(),
						NULL, NULL,
						REG_OPTION_NON_VOLATILE,
						KEY_READ | KEY_WRITE,
						NULL,
						handle, &disposition);

					reg::assert::success(result);
				}
				return { handle, static_cast<Disposition>(disposition) };
			}

			/// <summary>Creates a new value under the given key and sets its data.<para/>
			/// Throws an exception if the key cannot be opened or the value cannot be set</summary>
			/// <param name='machine'>Handle to an open key</param>
			/// <param name='value'>Name of the value to be created</param>
			/// <param name='data'>Data to be set for the created value</param>
			/// <returns>A tuple containing a handle to the opened key and
			/// a value that can only be REG_CREATED_NEW_KEY, REG_OPENED_EXISTING_KEY or NULL</returns>
			template<typename T>
			Disposition _create_value(HKEY handle, std::string_view value, T data)
			{
				DWORD disposition = NULL;
				if (reg::value_exists(handle, value))
					disposition = REG_OPENED_EXISTING_KEY;
				else
					disposition = REG_CREATED_NEW_KEY;

				reg::update::_set_data(handle, value, data);

				return static_cast<Disposition>(disposition);
			}

			/// <summary>Creates a new value under the given key and sets its data.<para/>
			/// Throws an exception if the key cannot be opened or the value cannot be set</summary>
			/// <param name='machine'>Root key in the hierarchy</param>
			/// <param name='key'>Subkey to the desired node</param>
			/// <param name='value'>Name of the value to be created</param>
			/// <param name='data'>Data to be set for the created value</param>
			/// <returns>A tuple containing a handle to the opened key and
			/// a value that can only be REG_CREATED_NEW_KEY, REG_OPENED_EXISTING_KEY or NULL</returns>
			template<typename T>
			std::tuple<PHKEY, Disposition> _create_value(HKEY machine, std::string_view key, std::string_view value, T data)
			{
				PHKEY handle = reg::open(machine, key, KEY_READ | KEY_WRITE);
				Disposition disposition = _create_value(*handle, value, data);
				return { handle, disposition };
			}

			/// <summary>Creates a new value under the given key and sets its data.
			/// If they key does not exist, the function first creates the key
			/// and then creates the value under it.<para/>
			/// Returns a handle to the key and a variable indicating if the key had
			/// to be created or if the value had to be created.<para/>
			/// Throws an exception if the key cannot be created or opened.</summary>
			/// <param name='machine'>Root key in the hierarchy</param>
			/// <param name='key'>Subkey to the desired node</param>
			/// <param name='value'>Name of the value to be created</param>
			/// <param name='data'>Data to be set for the value</param>
			/// <returns>A handle to an open key and a variable indicating
			/// whether the key had to be created or the value had to be created.</returns>
			template<typename T>
			std::tuple<PHKEY, Disposition> item(HKEY machine, std::string_view key, std::string_view value, T data) {
				if (key_exists(machine, key))
				{
					auto handle = reg::open(machine, key, KEY_WRITE);

					if (value_exists(machine, key, value))
						return { handle, Disposition::EXISTS_VALUE };

					else
					{
						reg::create::_create_value<T>(*handle, value, data);
						return { handle, Disposition::CREATED_VALUE };
					}
				}
				else
				{
					auto handle_and_disposition = reg::create::_create_key(machine, key);
					reg::create::_create_value<T>(*std::get<0>(handle_and_disposition), value, data);
					return handle_and_disposition;
				}
			}
		}

		/// <summary>Creates a new key. If the key already exists, the function opens it.<para/>
		/// Returns a handle to the open key and a variable indicating whether the key had to
		/// be open or created.<para/>
		/// Throws an exception if the key cannot be open/created.</summary>
		/// <param name='machine'>Root key in the hierarchy</param>
		/// <param name='key'>Subkey to the desired node</param>
		inline std::tuple<PHKEY, Disposition> key(HKEY machine, std::string_view key) {
			return reg::create::_create_key(machine, key);
		}

		/// <summary>Creates a new number value and assigns it the given data.
		/// If the specified key does not exist, the function creates it.<para/>
		/// Returns a handle to the open key and a variable indicating whether
		/// the key had to be created or the value had to be created.<para/>
		/// Throws an exception if the key cannot be open/created</summary>
		/// <param name='machine'>Root key in the hierarchy</param>
		/// <param name='key'>Subkey to the desired node</param>
		/// <param name='value'>Name of the value to be created</param>
		/// <param name='data'>Data to be assigned to value</param>
		inline std::tuple<PHKEY, Disposition> number(HKEY machine, std::string_view key, std::string_view value, DWORD data = 0) {
			return reg::create::item<DWORD>(machine, key, value, data);
		}

		/// <summary>Creates a new string value and assigns it the given data.
		/// If the specified key does not exist, the function creates it.<para/>
		/// Returns a handle to the open key and a variable indicating whether
		/// the key had to be created or the value had to be created.<para/>
		/// Throws an exception if the key cannot be open/created</summary>
		/// <param name='machine'>Root key in the hierarchy</param>
		/// <param name='key'>Subkey to the desired node</param>
		/// <param name='value'>Name of the value to be created</param>
		/// <param name='data'>Data to be assigned to value</param>
		inline std::tuple<PHKEY, Disposition> string(HKEY machine, std::string_view key, std::string_view value, std::string_view data = "") {
			return reg::create::item<std::string_view>(machine, key, value, data);
		}
	}

	// delete is a keyword :/
	namespace remove
	{
		namespace
		{
			/// <summary>Removes the key associated with the handle.<para/>
			/// Throws an exception if the function fails</summary>
			/// <param name='handle'>A handle to an open registry key</param>
			void _remove_key(HKEY handle)
			{
				DWORD code = NULL;
				code = RegDeleteKeyEx(handle, "", KEY_WOW64_64KEY, NULL);

				reg::assert::success(code);
			}

			/// <summary>Removes the specified key.<para/>
			/// Throws an exception if the function fails</summary>
			/// <param name='machine'>Root key in the hierarchy</param>
			/// <param name='key'>Subkey to the desired node</param>
			void _remove_key(HKEY machine, std::string_view key)
			{
				auto handle = self_closing_handle(reg::open(machine, key, DELETE));
				reg::remove::_remove_key(*handle);
			}

			/// <summary>Deletes the subkeys and values of the specified key recursively.</summary>
			/// <param name='handle'>A handle to an open registry key</param>
			void _remove_children(HKEY handle)
			{
				DWORD code = NULL;
				code = RegDeleteTree(handle, "");
				reg::assert::success(code);
			}

			/// <summary>Deletes the subkeys and values of the specified key recursively.</summary>
			/// <param name='machine'>Root key in the hierarchy</param>
			/// <param name='key'>Subkey to the desired node</param>
			void _remove_children(HKEY machine, std::string_view key)
			{
				auto handle = reg::self_closing_handle(
					reg::open(
						machine,
						key,
						DELETE |
						KEY_ENUMERATE_SUB_KEYS |
						KEY_QUERY_VALUE |
						KEY_SET_VALUE));

				reg::remove::_remove_children(*handle);
			}

			/// <summary>Removes a value under a registry key</summary>
			/// <param name='handle'>A handle to an open registry key</param>
			/// <param name='value'>Name of the value to be removed</param>
			void _remove_value(HKEY handle, std::string_view value)
			{
				DWORD code = NULL;
				code = RegDeleteValue(handle, value.data());

				reg::assert::success(code);
			}

			/// <summary>Removes a value under a registry key</summary>
			/// <param name='machine'>Root key in the hierarchy</param>
			/// <param name='key'>Subkey to the desired node</param>
			/// <param name='value'>Name of the value to be removed</param>
			void _remove_value(HKEY machine, std::string_view key, std::string_view value)
			{
				auto handle = self_closing_handle(reg::open(machine, key, KEY_SET_VALUE));
				reg::remove::_remove_value(*handle, value);
			}
		}

		/// <summary>Removes a key from the registry. If the key does not exist,
		/// the function returns false.<para/>
		/// Throws an exception if the key cannot be removed.</summary>
		/// <param name='machine'>Root key in the hierarchy</param>
		/// <param name='key'>Subkey to the desired node</param>
		/// <returns>True, if the key was removed. False otherwise</returns>
		inline bool key(HKEY machine, std::string_view key)
		{
			if (key_exists(machine, key))
			{
				reg::remove::_remove_key(machine, key);
				return true;
			}
			else
				return false;
		}

		/// <summary>Removes all subkeys of the given key. 
		/// The given key and its values remain unchanged.</summary>
		/// <param name='machine'>Root key in the hierarchy</param>
		/// <param name='key'>Subkey to the desired node</param>
		/// <returns>True, if at least one subkey was removed.
		/// False, if no subkeys were removed or the given key does not exist.</returns>
		inline bool subkeys(HKEY machine, std::string_view key)
		{
			if (key_exists(machine, key))
			{
				auto handle = self_closing_handle(reg::open(machine, key, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS));

				auto keys = reg::query::keys(*handle);
				for (const std::string& key_name : keys)
				{
					reg::remove::_remove_children(*handle, key_name);
					reg::remove::_remove_key(*handle, key_name);
				}
				return !keys.empty();
			}
			else
				return false;
		}

		/// <summary>Removes all values of the given key.
		/// The subkeys and their values remain unchanged.</summary>
		/// <param name='machine'>Root key in the hierarchy</param>
		/// <param name='key'>Subkey to the desired node</param>
		/// <returns>True, if at least one value was removed.
		/// False if no values were removed or the given key does not exist.</returns>
		inline bool values(HKEY machine, std::string_view key)
		{
			if (key_exists(machine, key))
			{
				auto handle = reg::self_closing_handle(reg::open(machine, key, KEY_SET_VALUE | KEY_QUERY_VALUE));

				auto value_names = reg::query::value_names(*handle);
				for (const std::string& name : value_names)
					reg::remove::_remove_value(*handle, name);

				return !value_names.empty();
			}
			else
				return false;
		}

		/// <summary>Removes a key recursively. The key, all of its subkeys
		/// and all of its values are removed.</summary>
		/// <param name='machine'>Root key in the hierarchy</param>
		/// <param name='key'>Subkey to the desired node</param>
		/// <returns>True, if the key was removed. False otherwise</returns>
		inline bool cluster(HKEY machine, std::string_view key)
		{
			if (key_exists(machine, key))
			{
				reg::remove::_remove_children(machine, key);
				reg::remove::key(machine, key);
				return true;
			}
			else
				return false;
		}

		/// <summary>Removes a value from a registry key. If the value does not exist,
		/// the function returns false.<para/>
		/// Throws an exception if the value cannot be removed.</summary>
		/// <param name='machine'>Root key in the hierarchy</param>
		/// <param name='key'>Subkey to the desired node</param>
		/// <param name='value'>Name of the value to be removed</param>
		/// <returns>True, if the value was removed. False otherwise</returns>
		inline bool value(HKEY machine, std::string_view key, std::string_view value)
		{
			if (key_exists(machine, key))
				if (value_exists(machine, key, value))
				{
					reg::remove::_remove_value(machine, key, value);
					return true;
				}

			return false;
		}
	}
}
