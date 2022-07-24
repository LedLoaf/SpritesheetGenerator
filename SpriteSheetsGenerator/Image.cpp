#include "Image.h"

Image::Image(const std::string& filename, 
			 const std::string& name, 
			 const size_t x, const size_t y, 
			 const size_t width, const size_t height, 
			 const size_t rotation)
{
	this->m_filename = filename;
	this->m_name = name;
	this->m_x = x;
	this->m_y = y;
	this->m_width = width;
	this->m_height = height;
	this->m_rotation = rotation;
}

std::string Image::getName() {
	return m_name;
}

size_t Image::getX() {
	return m_x;
}
size_t Image::getY() {
	return m_y;
}
size_t Image::getWidth() {
	return m_width;
}
size_t Image::getHeight() {
	return m_height;
}

size_t Image::getRotation() {
	return m_rotation;
}

std::string Image::getFilename() {
	return m_filename;
}