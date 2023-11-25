export module IMemoire;

import <cstdint>;

export using octet = std::uint8_t;
export using adresse = std::uint16_t;

export class IMemoire
{
public:
	virtual ~IMemoire() = default;
	virtual octet* ptr(adresse adr) = 0;
	virtual const octet* ptr(adresse adr) const = 0;
	virtual int taille() const = 0;
public:
	const octet& operator[](adresse adr) const { return *ptr(adr); }
	octet& operator[](adresse adr) { return *ptr(adr); }
};
