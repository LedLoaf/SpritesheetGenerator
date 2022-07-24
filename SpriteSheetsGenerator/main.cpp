/*
	I will use only one algorithm int the file 'MaxRectsBinPack.h' and 'MaxRectsBinPack.cpp' with multiple heuristics to make the code simpler to understand.

	To sum up the things we need to do are:
		1. Load all the images
		2. Try to pack every image into a pack of rectangles
		3. Check if the pack is valid and render the pack into a png image
		4. Save a XML file with one entry, for every file (to have names, positions and sizes of every images)

	I should mention that the algorithm for the packing can rotate the rectangles so we will need to put that in the XML file as well.

	Things needed:
		1. rapidXML			http://sourceforge.net/projects/rapidxml/files/
		2. SFML/SDL2
		3. RectangleBinPack https://github.com/juj/RectangleBinPack
		4. dirent.h

	We will use the library of Jukka Jylänki released under public domain : 'RectangleBinPack'
	Texture packing problem also known as Bin packing problem is NP-Hard.

	If you want to understand the algorithm before using them, you can read the articles written by the author of the library :

	Rectangle Bin Packing
	http://web.archive.org/web/20180812043418/http://clb.demon.fi/projects/rectangle-bin-packing
	More Rectangle Bin Packing
	http://web.archive.org/web/20180812040822/http://clb.demon.fi/projects/more-rectangle-bin-packing
	Even More Rectangle Bin Packing
	http://web.archive.org/web/20180812042647/http://clb.demon.fi/projects/even-more-rectangle-bin-packing

	NOTE* If everything is implemented, you may need to include '<algorithm>' in the MaxRectsBinPack.cpp to have 'min' and 'max'
*/
#include <fstream>
#include <iostream>
#include <rapidxml.hpp>
#include <rapidxml_ext.hpp>
#include <SFML/Graphics.hpp>
#include "Image.h"
#include "MaxRectsBinPack.h"
#include "dirent.h"

const char* toStr(const size_t value)
{
	const std::string str = std::to_string(value);
	return str.c_str();
}

/*
   Return the filename of every files in a folder, this method only works on Windows.
   If you want to use this function you need to change the path of the filename and include the file "dirent.h"
*/
std::vector<std::string> getListFiles(const std::string& filepath)
{
	std::vector<std::string> list;
	if (DIR* dir; (dir = opendir(filepath.c_str())) != nullptr) {
		struct dirent* ent;
		// Print all the files and directories within directory
		while ((ent = readdir(dir)) != nullptr) {

			if (std::string str = ent->d_name; str != "." && str != "..") list.push_back(str);
		}
		closedir(dir);
	}
	return list;
}

/* The next function chooseBestHeuristic try every heuristics for the algorithm and pick the best one by comparing occupancy. */
rbp::MaxRectsBinPack::FreeRectChoiceHeuristic chooseBestHeuristic(const std::vector<sf::Texture*>* rects, const size_t texWidth, const size_t texHeight)
{
	rbp::MaxRectsBinPack                                       pack;
	std::vector<rbp::MaxRectsBinPack::FreeRectChoiceHeuristic> listHeuristics;
	listHeuristics.push_back(rbp::MaxRectsBinPack::RectBestAreaFit);
	listHeuristics.push_back(rbp::MaxRectsBinPack::RectBestLongSideFit);
	listHeuristics.push_back(rbp::MaxRectsBinPack::RectBestShortSideFit);
	listHeuristics.push_back(rbp::MaxRectsBinPack::RectBottomLeftRule);
	listHeuristics.push_back(rbp::MaxRectsBinPack::RectContactPointRule);

	rbp::MaxRectsBinPack::FreeRectChoiceHeuristic heuristicRule{};
	float                                         max = 0;

	for (const auto& heuristic : listHeuristics) {
		pack.init(static_cast<int>(texWidth), static_cast<int>(texHeight));

		for (const auto texture : *rects) {
			pack.insert(static_cast<int>(texture->getSize().x), static_cast<int>(texture->getSize().y), heuristic);
		}

		if (pack.occupancy() > max) {
			max           = pack.occupancy();
			heuristicRule = heuristic;
		}
	}
	return heuristicRule;
}

/* The next functions getXMLSheet generate the xml document from the data. */
std::string getXmlSheet(std::vector<Image> images, const std::string& name)
{
	rapidxml::xml_document<> doc;

	rapidxml::xml_node<>* root = doc.allocate_node(rapidxml::node_element, "TextureList");
	root->append_attribute(doc.allocate_attribute("Filename", doc.allocate_string(name.c_str())));
	doc.append_node(root);

	for (auto& img : images) {
		rapidxml::xml_node<>* child = doc.allocate_node(rapidxml::node_element, "image");
		child->append_attribute(doc.allocate_attribute("name", doc.allocate_string(img.getName().c_str())));
		child->append_attribute(doc.allocate_attribute("x", doc.allocate_string(toStr(img.getX()))));
		child->append_attribute(doc.allocate_attribute("y", doc.allocate_string(toStr(img.getY()))));
		child->append_attribute(doc.allocate_attribute("w", doc.allocate_string(toStr(img.getWidth()))));
		child->append_attribute(doc.allocate_attribute("h", doc.allocate_string(toStr(img.getHeight()))));

		if (img.getRotation() != 0) {
			child->append_attribute(doc.allocate_attribute("rotation", doc.allocate_string(toStr(img.getRotation()))));
		}

		root->append_node(child);
	}

	std::string xmlAsString;
	print(std::back_inserter(xmlAsString), doc, 0);

	return xmlAsString;
}

int main()
{
	std::vector<sf::Texture*> imgTex;					// images textures
	std::vector<std::string>  imgTexID;					// name of the images
	std::vector<Image>        images;					// xml data of the images
	std::string               filename = "sheet";		// filename of the sprite sheet
	sf::Vector2i              size(512, 512);		// size of the sprite sheet

	sf::RenderTexture rend;								// texture to render the sprite sheet
	rend.create(size.x, size.y);

	rbp::MaxRectsBinPack pack(size.x, size.y); //pack of image

	const std::string filepath = R"(C:\Users\LedLo\Downloads\_Glusoft\SpriteSheetsGenerator\SpriteSheetsGenerator\images\)";
	// List all filename's in the folder images

	// Load all the images
	for (std::vector<std::string> listAll = getListFiles(filepath); auto& img : listAll) {
		auto* texture = new sf::Texture();
		texture->loadFromFile("images/" + img);
		imgTex.push_back(texture);
		imgTexID.push_back(img.substr(0, listAll.size() - 4));
	}

	float rotation = 0;

	// Choose the best heuristic
	const rbp::MaxRectsBinPack::FreeRectChoiceHeuristic best1 = chooseBestHeuristic(&imgTex, size.x, size.y);

	for (size_t i = 0; i < imgTex.size(); i++) {
		// Insert the image into the pack
		rbp::Rect packedRect = pack.insert(static_cast<int>(imgTex[i]->getSize().x), static_cast<int>(imgTex[i]->getSize().y), best1);

		if (packedRect.height <= 0) {
			std::cout << "Error: The pack is full\n";
		}

		sf::Sprite spr(*imgTex[i]);			// sprite to draw on the render texture

		// If the image is rotated
		if (static_cast<int>(imgTex[i]->getSize().x) == packedRect.height && packedRect.width != packedRect.height) {
			rotation = 90;					// set the rotation for the xml data

			// Rotate the sprite to draw
			size_t oldHeight = spr.getTextureRect().height;
			spr.setPosition(static_cast<float>(packedRect.x), static_cast<float>(packedRect.y));
			spr.rotate(rotation);
			spr.setPosition(spr.getPosition().x + static_cast<float>(oldHeight), spr.getPosition().y);
		}
		else { // If there is no rotation
			rotation = 0;
			spr.setPosition(static_cast<float>(packedRect.x), static_cast<float>(packedRect.y));
		}

		rend.draw(spr);					// draw the sprite on the sprite sheet
		// Save data of the image for the xml file
		images.emplace_back(filename, imgTexID[i], packedRect.x, packedRect.y, packedRect.width, packedRect.height, static_cast<size_t>(rotation));
	}

	rend.display();						// render the texture properly

	// Free the memory of the images
	for (auto& tex : imgTex) {
		delete(tex);
	}

	// Save the sprite sheet
	sf::Texture tex = rend.getTexture();
	sf::Image   img = tex.copyToImage(); // need to create an image to save a file
	img.saveToFile("sheets/" + filename + ".png");

	// Generate the xml document
	std::string xml = getXmlSheet(images, filename + ".png");
	std::cout << xml; // display the xml file in the console

	// Save the XML document
	std::ofstream xmlFile;
	xmlFile.open("sheets/" + filename + ".xml");
	xmlFile << xml;
	xmlFile.close();

	// See the occupancy of the packing
	std::cout << "pack1 : " << pack.occupancy() << "%\n";

	// SFML code the create a window and display the sprite sheet
	sf::RenderWindow window(sf::VideoMode(size.x, size.y), "Sprite sheets generator");
	sf::Sprite       spr(tex);

	while (window.isOpen()) {
		sf::Event event{};
		while (window.pollEvent(event)) {
			if (event.type == sf::Event::Closed) window.close();
		}

		window.clear(sf::Color::White);
		window.draw(spr);
		window.display();
	}
}
