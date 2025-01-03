#pragma once

#include <string>

// it's name can be registered with the collection
class ImGuiPanel
{
	friend class ImGuiService;
public:
	virtual void OnImGui() = 0;
	inline const std::string& Name() { return _name; }
private:
	std::string _name;
};