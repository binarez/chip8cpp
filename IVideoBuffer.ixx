export module IVideoBuffer;

// 1 bit par pixel video buffer
export class IVideoBuffer
{
public:
	virtual ~IVideoBuffer() = default;
	virtual int hauteur() const = 0;
	virtual int largeur() const = 0;
	virtual bool pixel(int x, int y) const = 0;
	virtual void pixelXOR(int x, int y, bool value) = 0;
	virtual void reset() = 0;
};
