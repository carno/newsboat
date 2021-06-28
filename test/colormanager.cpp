#include "colormanager.h"

#include <string>
#include <unordered_set>
#include <vector>

#include "3rd-party/catch.hpp"

#include "confighandlerexception.h"
#include "configparser.h"

using namespace newsboat;

class StylesCollector {
	std::map<std::string, std::string> styles;

public:
	StylesCollector() = default;

	std::function<void(const std::string&, const std::string&)> setter()
	{
		return [this](const std::string& element, const std::string& style) {
			if (this->styles.find(element) != this->styles.cend()) {
				throw std::invalid_argument(std::string("Multiple styles for element ") + element);
			}

			this->styles[element] = style;
		};
	}

	size_t styles_count() const
	{
		return styles.size();
	}

	std::string style(const std::string& element) const
	{
		const auto style = styles.find(element);
		if (style != styles.cend()) {
			return style->second;
		} else {
			return {};
		}
	}
};

TEST_CASE(
	"apply_colors() invokes the callback for each element, supplying the element name and its style",
	"[ColorManager]")
{
	ColorManager c;
	StylesCollector collector;

	SECTION("By default, the list is empty") {
		c.apply_colors(collector.setter());
		REQUIRE(collector.styles_count() == 0);
	}

	SECTION("Each processed action adds corresponding entry to return value") {
		c.handle_action("color", {"listnormal", "default", "default"});
		c.handle_action("color", {"listfocus_unread", "cyan", "default", "bold", "underline"});
		c.handle_action("color", {"background", "red", "yellow"});
		c.handle_action("color", {"info", "green", "white", "reverse"});
		c.handle_action("color", {"end-of-text-marker", "color123", "default", "dim", "protect"});

		c.apply_colors(collector.setter());

		REQUIRE(collector.styles_count() == 5);
		REQUIRE(collector.style("listnormal") == "");
		REQUIRE(collector.style("listfocus_unread") == "fg=cyan,attr=bold,attr=underline");
		REQUIRE(collector.style("background") == "fg=red,bg=yellow");
		REQUIRE(collector.style("info") == "fg=green,bg=white,attr=reverse");
		REQUIRE(collector.style("end-of-text-marker") == "fg=color123,attr=dim,attr=protect");
	}

	SECTION("For `article` element, two additional elements are emitted") {
		c.handle_action("color", {"article", "white", "blue", "reverse"});

		c.apply_colors(collector.setter());

		REQUIRE(collector.styles_count() == 3);
		REQUIRE(collector.style("article") == "fg=white,bg=blue,attr=reverse");
		REQUIRE(collector.style("color_bold") == "fg=white,bg=blue,attr=reverse,attr=bold");
		REQUIRE(collector.style("color_underline") ==
			"fg=white,bg=blue,attr=reverse,attr=underline");
	}
}

TEST_CASE("register_commands() registers ColorManager with ConfigParser",
	"[ColorManager]")
{
	ConfigParser cfg;
	ColorManager clr;

	StylesCollector collector;

	REQUIRE_NOTHROW(clr.register_commands(cfg));

	clr.apply_colors(collector.setter());
	REQUIRE(collector.styles_count() == 0);

	cfg.parse_file("data/config-with-colors");

	clr.apply_colors(collector.setter());
	REQUIRE(collector.styles_count() == 2);
}

TEST_CASE(
	"handle_action() throws ConfigHandlerException if there aren't "
	"enough parameters",
	"[ColorManager]")
{
	ColorManager c;

	CHECK_THROWS_AS(c.handle_action("color", {}), ConfigHandlerException);
	CHECK_THROWS_AS(
		c.handle_action("color", {"one"}), ConfigHandlerException);
	CHECK_THROWS_AS(c.handle_action("color", {"one", "two"}),
		ConfigHandlerException);
}

TEST_CASE(
	"handle_action() throws ConfigHandlerException if foreground color "
	"is invalid",
	"[ColorManager]")
{
	ColorManager c;

	const std::vector<std::string> non_colors{
		{"awesome", "but", "nonexistent", "colors"}};
	for (const auto& color : non_colors) {
		CHECK_THROWS_AS(c.handle_action("color",
		{"listfocus", color, "default"}),
		ConfigHandlerException);
	}
}

TEST_CASE(
	"handle_action() throws ConfigHandlerException if background color "
	"is invalid",
	"[ColorManager]")
{
	ColorManager c;

	const std::vector<std::string> non_colors{
		{"awesome", "but", "nonexistent", "colors"}};
	for (const auto& color : non_colors) {
		CHECK_THROWS_AS(c.handle_action("color",
		{"listfocus", "default", color}),
		ConfigHandlerException);
	}
}

TEST_CASE(
	"handle_action() throws ConfigHandlerException if color attribute "
	"is invalid",
	"[ColorManager]")
{
	ColorManager c;

	const std::vector<std::string> non_attributes{
		{"awesome", "but", "nonexistent", "attributes"}};
	for (const auto& attr : non_attributes) {
		CHECK_THROWS_AS(c.handle_action("color",
		{"listfocus", "red", "red", attr}),
		ConfigHandlerException);
	}
}

TEST_CASE(
	"handle_action() throws ConfigHandlerException if color is applied "
	"to non-existent element",
	"[ColorManager]")
{
	ColorManager c;

	const std::vector<std::string> non_elements{
		{"awesome", "but", "nonexistent", "elements"}};
	for (const auto& element : non_elements) {
		CHECK_THROWS_AS(
			c.handle_action("color", {element, "red", "green"}),
			ConfigHandlerException);
	}
}

TEST_CASE(
	"handle_action() throws ConfigHandlerException if it's passed a "
	"command other than \"color\"",
	"[ColorManager]")
{
	ColorManager c;

	const std::vector<std::string> other_commands{
		{"browser", "include", "auto-reload", "ocnews-flag-star"}};
	for (const auto& command : other_commands) {
		CHECK_THROWS_AS(
			c.handle_action(command, {}), ConfigHandlerException);
	}
}

TEST_CASE("dump_config() returns everything we put into ColorManager",
	"[ColorManager]")
{
	ColorManager c;

	std::unordered_set<std::string> expected;
	std::vector<std::string> config;

	// Checks that `expected` contains the same lines as `config` contains,
	// and nothing more.
	auto equivalent = [&]() -> bool {
		std::size_t found = 0;
		for (const auto& line : config)
		{
			if (expected.find(line) == expected.end()) {
				return false;
			}

			found++;
		}

		return found == expected.size();
	};

	{
		INFO("Empty ColorManager outputs nothing");
		c.dump_config(config);
		REQUIRE(config.empty());
		REQUIRE(equivalent());
	}

	expected.emplace("color listfocus default red");
	c.handle_action("color", {"listfocus", "default", "red"});
	config.clear();
	c.dump_config(config);
	REQUIRE(config.size() == 1);
	REQUIRE(equivalent());

	expected.emplace("color article green cyan bold");
	c.handle_action("color", {"article", "green", "cyan", "bold"});
	config.clear();
	c.dump_config(config);
	REQUIRE(config.size() == 2);
	REQUIRE(equivalent());

	expected.emplace("color listnormal_unread black yellow underline standout");
	c.handle_action("color",
	{"listnormal_unread", "black", "yellow", "underline", "standout"});
	config.clear();
	c.dump_config(config);
	REQUIRE(config.size() == 3);
	REQUIRE(equivalent());
}
