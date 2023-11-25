export module Memoire;

export import IMemoire;

// La mémoire est un tableau d'octets.
// La taille est définie avec le type adresse puisque c'est la taille maximale adressable par le type adresse.
// Cela permet de s'assurer que la taille de la mémoire ne dépasse pas la taille maximale adressable.

export class Memoire : public IMemoire
{
public:
	Memoire(adresse taille)	// taille en octets
		: data_(new octet[taille])
		, taille_{ taille }
	{
	}
	~Memoire()
	{
		delete[] data_;
	}
public: // IMemoire
	virtual int taille() const { return taille_; }	// octets
	virtual octet * ptr(adresse adr) { return data_ + adr; }
	virtual const octet* ptr(adresse adr) const { return data_ + adr; }
protected:
	octet* data_{ nullptr };
	adresse taille_{ 0 };
};
