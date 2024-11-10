#pragma once

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <XEnum.h>
#include <functional>

namespace ImGui
{
#pragma region XEnumExtensions
	/*
	template<typename T> requires XEnumTraits<T>::is_valid
	bool EnumCombo(const char* const label, T& value, ImGuiComboFlags flags = 0)
	{
		auto origValue = value;

		if (ImGui::BeginCombo(label, XEnumTraits<T>::ToCString(value), flags))
		{
			for (const auto& option : XEnumTraits<T>::Values)
			{
				bool is_selected = option == value; 
				if (ImGui::Selectable(XEnumTraits<T>::ToCString(option), is_selected))
					value = option;
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}

			ImGui::EndCombo();
		}
		
		return origValue != value;
	}
	*/

	template <XEnumValue T>
	bool Combo(const char* const label, T& value, ImGuiComboFlags flags = 0)
	{
		auto origValue = value;

		if (ImGui::BeginCombo(label, XEnumTraits<T>::ToCString(value), flags))
		{
			for (const auto& option : XEnumTraits<T>::Values)
			{
				bool is_selected = option == value;
				if (ImGui::Selectable(XEnumTraits<T>::ToCString(option), is_selected))
					value = option;
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}

			ImGui::EndCombo();
		}

		return origValue != value;
	}

	template <typename T> requires XEnumTraits<T>::is_valid
	bool Combo(T& value, ImGuiComboFlags flags = 0)
	{
		return Combo(XEnumTraits<T>::name, value, flags);
	}
#pragma endregion

	struct Scope
	{
		std::function<void()> _release = nullptr;

		Scope(const std::function<void()>& release) {
			_release = release;
		}
		constexpr Scope(Scope&&) = default;
		constexpr Scope(const Scope& other) = default;
		~Scope() {
			if (_release)
				std::invoke(_release);
		}

		operator bool() const { return _release != nullptr; }
	};
}

// Scoped Tree Nodes
namespace ImGui::Scoped
{
	namespace details
	{
		[[nodiscard]] inline Scope GenerateTreeNodeScope(bool enabled) {
			if (enabled)
				return Scope([]() -> void { ImGui::TreePop(); });
			return Scope(nullptr);
		}
	}

	[[nodiscard]] inline Scope TreeNode(const char* str_id, const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		bool is_open = ImGui::TreeNodeExV(str_id, 0, fmt, args);
		va_end(args);
		return details::GenerateTreeNodeScope(is_open);
	}

	[[nodiscard]] inline Scope TreeNode(const void* ptr_id, const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		bool is_open = ImGui::TreeNodeExV(ptr_id, 0, fmt, args);
		va_end(args);
		return details::GenerateTreeNodeScope(is_open);
	}

	[[nodiscard]] inline Scope TreeNode(const char* label)
	{
		return details::GenerateTreeNodeScope(ImGui::TreeNode(label));
	}

	[[nodiscard]] inline Scope TreeNodeV(const char* str_id, const char* fmt, va_list args)
	{
		return details::GenerateTreeNodeScope(ImGui::TreeNodeExV(str_id, 0, fmt, args));
	}

	[[nodiscard]] inline Scope TreeNodeV(const void* ptr_id, const char* fmt, va_list args)
	{
		return details::GenerateTreeNodeScope(ImGui::TreeNodeExV(ptr_id, 0, fmt, args));
	}

	[[nodiscard]] inline Scope TreeNodeEx(const char* label, ImGuiTreeNodeFlags flags)
	{
		return details::GenerateTreeNodeScope(ImGui::TreeNodeEx(label, flags));
	}

	[[nodiscard]] inline Scope TreeNodeEx(const char* str_id, ImGuiTreeNodeFlags flags, const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		bool is_open = ImGui::TreeNodeExV(str_id, flags, fmt, args);
		va_end(args);
		return details::GenerateTreeNodeScope(is_open);
	}

	[[nodiscard]] inline Scope TreeNodeEx(const void* ptr_id, ImGuiTreeNodeFlags flags, const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		bool is_open = ImGui::TreeNodeExV(ptr_id, flags, fmt, args);
		va_end(args);
		return details::GenerateTreeNodeScope(is_open);
	}

	[[nodiscard]] inline Scope TreeNodeExV(const char* str_id, ImGuiTreeNodeFlags flags, const char* fmt, va_list args)
	{
		return details::GenerateTreeNodeScope(ImGui::TreeNodeExV(str_id, flags, fmt, args));
	}

	[[nodiscard]] inline Scope TreeNodeExV(const void* ptr_id, ImGuiTreeNodeFlags flags, const char* fmt, va_list args)
	{
		return details::GenerateTreeNodeScope(ImGui::TreeNodeExV(ptr_id, flags, fmt, args));
	}
}

// Scoped Ids
namespace ImGui::Scoped 
{
	[[nodiscard]] inline Scope Id(const char* str_id)
	{
		ImGui::PushID(str_id);
		return Scope([]() -> void { ImGui::PopID(); });
	}

	[[nodiscard]] static inline Scope Id(const std::string& view)
	{
		return Id(view.c_str());
	}

	[[nodiscard]] inline Scope Id(const char* str_id_begin, const char* str_id_end)
	{
		ImGui::PushID(str_id_begin, str_id_end);
		return Scope([]() -> void { ImGui::PopID(); });
	}

	[[nodiscard]] inline Scope Id(const void* ptr_id)
	{
		ImGui::PushID(ptr_id);
		return Scope([]() -> void { ImGui::PopID(); });
	}

	[[nodiscard]] inline Scope Id(int int_id)
	{
		ImGui::PushID(int_id);
		return Scope([]() -> void { ImGui::PopID(); });
	}
}

// Indents
namespace ImGui::Scoped
{
	[[nodiscard]] inline Scope Indent(float amount)
	{
		ImGui::Indent(amount);
		return Scope([amount]() -> void { ImGui::Unindent(amount); });
	}
}