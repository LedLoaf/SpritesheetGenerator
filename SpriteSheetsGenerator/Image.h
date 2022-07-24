#pragma once

#include <rapidxml.hpp>
#include <string>

// Image class for sheet
class Image {
public:
	Image(const std::string& filename, 
		  const std::string& name, 
		  const size_t x, const size_t y, 
		  const size_t width, const size_t height, 
		  const size_t rotation);

	~Image() = default;

	size_t getX();
	size_t getY();
	size_t getWidth();
	size_t getHeight();
	size_t getRotation();
	std::string getFilename();
	std::string getName();
private:
	std::string m_filename;
	std::string m_name;
	size_t m_x;
	size_t m_y;
	size_t m_width;
	size_t m_height;
	size_t m_rotation;
};
