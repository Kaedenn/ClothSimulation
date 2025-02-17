#pragma once
#include <cstdint>
#include <SFML/System.hpp>


struct Profiler
{
	struct Element
	{
		uint32_t start, total;
		Element()
			: start(0)
			, total(0)
		{}

		void reset()
		{
			start = 0;
			total = 0;
		}

		float asMilliseconds() const
		{
			return total * 0.001f;
		}
	};

	sf::Clock clock;

	Profiler()
	{
		clock.restart();
	}

	void start(Element& elem)
	{
		elem.start = clock.getElapsedTime().asMicroseconds();
	}

	void stop(Element& elem)
	{
		elem.total += clock.getElapsedTime().asMicroseconds() - elem.start;
	}
};

/* vim: set ts=4 sts=4 sw=4 et: */
