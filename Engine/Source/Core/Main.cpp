#include <WinkEngine/pch.hpp>
#include <WinkEngine/Core/Application.hpp>

extern std::unique_ptr<Wink::Application> create_application();

int main()
{
	auto app = create_application();
	if (app) app->run();
}