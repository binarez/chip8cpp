export module VideoBuffer;

export import IVideoBuffer;

import <cstdint>;
import <cassert>;
import <algorithm>;

export class VideoBuffer : public IVideoBuffer
{
public:
	VideoBuffer(int largeur, int hauteur)	// en pixels
		: pixels_(new bool[hauteur * largeur])
		, hauteur_{ hauteur }
		, largeur_{ largeur }
	{
		assert(hauteur > 0 && largeur > 0);
		reset();
	}
	~VideoBuffer()
	{
		delete[] pixels_;
	}

public: // IVideo
	virtual int hauteur() const
	{
		return hauteur_;
	}

	virtual int largeur() const
	{
		return largeur_;
	}

	virtual bool pixel(int x, int y) const
	{
		return pixels_[index(x, y)];
	}

	virtual void pixelXOR(int x, int y, bool value)
	{
		pixels_[index(x, y)] = pixels_[index(x, y)] != value;
	}

	virtual void reset()
	{
		std::for_each(pixels_, pixels_ + hauteur_ * largeur_, [](bool& pixel) { pixel = false; });
	}

protected:
	int index(int x, int y) const
	{
		assert(x >= 0 && x < largeur_);
		assert(y >= 0 && y < hauteur_);
		return y * largeur_ + x;
	}

protected:
	bool* pixels_{ nullptr };	// 1 bit par pixel. TODO: convertir en bitarray dynamique.
	int hauteur_{ 0 };
	int largeur_{ 0 };
};
