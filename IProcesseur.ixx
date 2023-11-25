export module IProcesseur;

import <string>;

export class IProcesseur
{
public:
	virtual ~IProcesseur() = default;
	virtual bool charger(const std::string& fichierRom) = 0;
	virtual void tick() = 0;
	virtual void appuyerToucheClavier(int touche) = 0;
	virtual void relacherToucheClavier(int touche) = 0;
};
