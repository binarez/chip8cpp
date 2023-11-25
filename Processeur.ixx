export module Processeur;

import <stack>;
import <iomanip>;
import <unordered_map>;
import <cassert>;
import <fstream>;
import <bitset>;

export import IProcesseur;
export import IMemoire;
export import IVideoBuffer;

// Une instance de Processeur par exécution seulement
export class Processeur : public IProcesseur
{
public:
	Processeur(IMemoire & memoire_, IVideoBuffer& video)
		: memoire_{ memoire_ }
		, video_{ video }
	{
		boot();
	}
	virtual ~Processeur()
	{}

public:
	virtual bool charger(const std::string& fichierRom)
	{
		// Directement @ eof avec std::ios::ate
		std::ifstream fichier{ fichierRom, std::ios::binary | std::ios::ate };
		if (!fichier.is_open())
			return false;

		// Obtenir la taille du fichier en demandant la position du curseur (qui est @ eof)
		std::streampos taille = fichier.tellg();

		// Vérifier que la taille du fichier est inférieure à la taille de la mémoire
		if (taille > (memoire_.taille() - Processeur::DEBUT_ZONE_ACCESSIBLE))
			return false;

		// Charger à partir du début du fichier dans la mémoire au début de la zone accessible
		fichier.seekg(0, std::ios::beg);
		fichier.read(reinterpret_cast<char*>(memoire_.ptr(Processeur::DEBUT_ZONE_ACCESSIBLE)), taille);
		fichier.close();
		return true;
	}

	virtual void tick()
	{
		// Fetch: lire l'instruction 16-bit @ PC
		uint16_t instruction{ static_cast<uint16_t>((memoire_[compteurProgramme_] << 8u) | memoire_[compteurProgramme_ + 1]) };

		// Avancer à la prochaine instruction le PC
		compteurProgramme_ += 2;

		// Décoder l'instruction et faire le look-up dans la table des opcodes
		const auto opcode{ versCleOpcode(instruction) };
		const auto itInstruction{ instructions.find(opcode) };
		assert(itInstruction != instructions.end());
		if (itInstruction != instructions.end())
		{
			((*this).*(itInstruction->second))(instruction);
		}

		// Idéalement décrémenter le chrono delai à 60Hz si > 0
		if (chronoDelai_ > 0)
		{
			--chronoDelai_;
		}

		// Idéalement décrémenter le chrono sonore à 60Hz si > 0
		if (chronoSon_ > 0)
		{
			--chronoSon_;
		}
	}

	virtual void appuyerToucheClavier(int touche)
	{
		clavier_.set(touche);
	}

	virtual void relacherToucheClavier(int touche)
	{
		clavier_.reset(touche);
	}

protected:
	using instruction = uint16_t;
	const static adresse DEBUT_ZONE_ACCESSIBLE{ 0x200 };

	void boot()
	{
		// The CHIP - 8 has 4096 bytes of memory, meaning the address space is from 0x000 to 0xFFF.
		// The address space is segmented into three sections :
		//  0x000 - 0x1FF : Originally reserved for the CHIP - 8 interpreter, but in our modern emulator we will just never write to or read from that area.Except for…
		//  0x050 - 0x0A0 : Storage space for the 16 built - in characters(0 through F), which we will need to manually put into our memory because ROMs will be looking for those characters.
		//  0x200 - 0xFFF : Instructions from the ROM will be stored starting at 0x200, and anything left after the ROM’s space is free to use.
		// src: https://austinmorlan.com/posts/chip8_emulator/#4k-bytes-of-memory
				
		// There are sixteen characters that ROMs expected at a certain location so they can write characters to the screen, so we need to put those characters into memory.
		// We need an array of these bytes to load into memory. There are 16 characters at 5 bytes each, so we need an array of 80 bytes.
		// src: https://austinmorlan.com/posts/chip8_emulator/#loading-the-fonts
		const unsigned int CARACS_TAILLE_EN_OCTETS = 16 * 5;
		uint8_t police[CARACS_TAILLE_EN_OCTETS] =
		{
			0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
			0x20, 0x60, 0x20, 0x20, 0x70, // 1
			0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
			0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
			0x90, 0x90, 0xF0, 0x10, 0x10, // 4
			0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
			0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
			0xF0, 0x10, 0x20, 0x40, 0x40, // 7
			0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
			0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
			0xF0, 0x90, 0xF0, 0x90, 0x90, // A
			0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
			0xF0, 0x80, 0x80, 0x80, 0xF0, // C
			0xE0, 0x90, 0x90, 0x90, 0xE0, // D
			0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
			0xF0, 0x80, 0xF0, 0x80, 0x80  // F
		};

		const adresse ADRESSE_POLICE_EN_MEMOIRE = 0x50;
		std::memcpy(memoire_.ptr(ADRESSE_POLICE_EN_MEMOIRE), police, CARACS_TAILLE_EN_OCTETS);
	}

	static uint16_t versCleOpcode(instruction op)
	{
		uint16_t demiOctet{ op & 0xF000u };
		switch (demiOctet)
		{
			case 0x8000u:
				return op & 0xF00Fu;
			case 0x0000u:
				return op;
			case 0xE000u:
			case 0xF000u:
				return op & 0xF0FFu;
			default:
				return op & 0xF000u;
		}
	}

protected: // Ensemble d'instructions

	void OP_CLS(instruction /*op*/)
	{
		video_.reset();
	}

	void OP_RET(instruction /*op*/)
	{
		compteurProgramme_ = pile_.top();
		pile_.pop();
	}

	void OP_JP(instruction op)
	{
		const adresse adr{ op & 0x0FFFu };
		compteurProgramme_ = adr;
	}

	void OP_CALL(instruction op)
	{
		const adresse adr{ op & 0x0FFFu };
		pile_.push(compteurProgramme_);
		compteurProgramme_ = adr;
	}

	void OP_SE_Vx_Valeur(instruction op)
	{
		const int Vx{ (op & 0x0F00u) >> 8u };
		const uint8_t valeur{ op & 0x00FFu };
		if (registres_[Vx] == valeur)
		{
			compteurProgramme_ += 2;
		}
	}

	void OP_SNE_Vx_Valeur(instruction op)
	{
		const int Vx{ (op & 0x0F00u) >> 8u };
		const uint8_t valeur{ op & 0x00FFu };
		if (registres_[Vx] != valeur)
		{
			compteurProgramme_ += 2;
		}
	}

	void OP_SE_Vx_Vy(instruction op)
	{
		const unsigned int Vx{ (op & 0x0F00u) >> 8u };
		const unsigned int Vy{ (op & 0x00F0u) >> 4u };
		if (registres_[Vx] == registres_[Vy])
		{
			compteurProgramme_ += 2;
		}
	}

	void OP_LD_Vx_Valeur(instruction op)
	{
		const unsigned int Vx{ (op & 0x0F00u) >> 8u };
		const uint8_t valeur{ op & 0x00FFu };
		registres_[Vx] = valeur;
	}

	void OP_LD_Vx_Vy(instruction op)
	{
		const unsigned int Vx{ (op & 0x0F00u) >> 8u };
		const unsigned int Vy{ (op & 0x00F0u) >> 4u };
		registres_[Vx] = registres_[Vy];
	}

	void OP_ADD_Vx_Valeur(instruction op)
	{
		const unsigned int Vx{ (op & 0x0F00u) >> 8u };
		const uint8_t valeur{ op & 0x00FFu };
		registres_[Vx] += valeur;
	}

	void OP_OR_Vx_Vy(instruction op)
	{
		const unsigned int Vx{ (op & 0x0F00u) >> 8u };
		const unsigned int Vy{ (op & 0x00F0u) >> 4u };
		registres_[Vx] |= registres_[Vy];
	}

	void OP_AND_Vx_Vy(instruction op)
	{
		const unsigned int Vx{ (op & 0x0F00u) >> 8u };
		const unsigned int Vy{ (op & 0x00F0u) >> 4u };
		registres_[Vx] &= registres_[Vy];
	}

	void OP_XOR_Vx_Vy(instruction op)
	{
		const unsigned int Vx{ (op & 0x0F00u) >> 8u };
		const unsigned int Vy{ (op & 0x00F0u) >> 4u };
		registres_[Vx] ^= registres_[Vy];
	}

	void OP_ADD_Vx_Vy(instruction op)
	{
		const unsigned int Vx{ (op & 0x0F00u) >> 8u };
		const unsigned int Vy{ (op & 0x00F0u) >> 4u };
		const uint16_t resultat{ static_cast<uint16_t>(registres_[Vx] + registres_[Vy]) };
		registres_[VF] = resultat > 255 ? 1 : 0;
		registres_[Vx] = resultat & 0xFFu;
	}

	void OP_SUB_Vx_Vy(instruction op)
	{
		const unsigned int Vx{ (op & 0x0F00u) >> 8u };
		const unsigned int Vy{ (op & 0x00F0u) >> 4u };
		registres_[VF] = registres_[Vx] > registres_[Vy] ? 1 : 0;
		registres_[Vx] -= registres_[Vy];
	}

	void OP_SHR_Vx(instruction op)
	{
		const unsigned int Vx{ (op & 0x0F00u) >> 8u };
		registres_[VF] = registres_[Vx] & 0x1u;
		registres_[Vx] >>= 1;
	}

	void OP_SUBN_Vx_Vy(instruction op)
	{
		const unsigned int Vx{ (op & 0x0F00u) >> 8u };
		const unsigned int Vy{ (op & 0x00F0u) >> 4u };
		registres_[VF] = registres_[Vy] > registres_[Vx] ? 1 : 0;
		registres_[Vx] = registres_[Vy] - registres_[Vx];
	}

	void OP_SHL_Vx(instruction op)
	{
		const unsigned int Vx{ (op & 0x0F00u) >> 8u };
		registres_[VF] = (registres_[Vx] & 0x80u) >> 7u;
		registres_[Vx] <<= 1;
	}

	void OP_SNE_Vx_Vy(instruction op)
	{
		const unsigned int Vx{ (op & 0x0F00u) >> 8u };
		const unsigned int Vy{ (op & 0x00F0u) >> 4u };
		if (registres_[Vx] != registres_[Vy])
		{
			compteurProgramme_ += 2;
		}
	}

	void OP_LD_I_Addr(instruction op)
	{
		const adresse adr{ op & 0x0FFFu };
		index_ = adr;
	}

	void OP_JP_V0_Addr(instruction op)
	{
		const adresse adr{ op & 0x0FFFu };
		compteurProgramme_ = registres_[V0] + adr;
	}

	void OP_RND_Vx_Valeur(instruction op)
	{
		const unsigned int Vx{ (op & 0x0F00u) >> 8u };
		const uint8_t valeur{ op & 0x00FFu };
		registres_[Vx] = (std::rand() % 256) & valeur;
	}

	void OP_DRW_Vx_Vy_Nibble(instruction op)
	{
		const unsigned int Vx{ (op & 0x0F00u) >> 8u };
		const unsigned int Vy{ (op & 0x00F0u) >> 4u };
		const int hauteur{ op & 0x000Fu };
		registres_[VF] = 0;

		for (int ligne = 0; ligne < hauteur; ++ligne)
		{
			const octet octetSprite{ memoire_[index_ + ligne] };
			for (int colonne = 0; colonne < 8; ++colonne)
			{
				const int pixelX{ static_cast<int>(registres_[Vx] + colonne) };
				const int pixelY{ static_cast<int>(registres_[Vy] + ligne) };
				const bool pixelPresentement{ video_.pixel(pixelX % video_.largeur(),pixelY % video_.hauteur()) };
				const bool spritePixel{ (octetSprite & (0x80u >> colonne)) != 0 };
				if (spritePixel)
				{
					if (pixelPresentement)
					{
						registres_[VF] = 1;
					}
					video_.pixelXOR(pixelX, pixelY, spritePixel);
				}
			}
		}
	}

	void OP_SKP_Vx(instruction op)
	{
		const unsigned int Vx{ (op & 0x0F00u) >> 8u };
		const octet touche{ registres_[Vx] };
		if (clavier_[touche])
		{
			compteurProgramme_ += 2;
		}
	}

	void OP_SKNP_Vx(instruction op)
	{
		const unsigned int Vx{ (op & 0x0F00u) >> 8u };
		const octet touche{ registres_[Vx] };
		if (!clavier_[touche])
		{
			compteurProgramme_ += 2;
		}
	}

	void OP_LD_Vx_DT(instruction op)
	{
		const unsigned int Vx{ (op & 0x0F00u) >> 8u };
		registres_[Vx] = chronoDelai_;
	}


	void OP_LD_Vx_K(instruction op)
	{
		const unsigned int Vx{ (op & 0x0F00u) >> 8u };
		const octet touche{ registres_[Vx] };
		if (clavier_[touche])
		{
			registres_[Vx] = touche;
		}
		else
		{
			// boucle tant que aucune touche n'est appuyée
			compteurProgramme_ -= 2;
		}
	}

	void OP_LD_DT_Vx(instruction op)
	{
		const unsigned int Vx{ (op & 0x0F00u) >> 8u };
		chronoDelai_ = registres_[Vx];
	}

	void OP_LD_ST_Vx(instruction op)
	{
		const unsigned int Vx{ (op & 0x0F00u) >> 8u };
		chronoSon_ = registres_[Vx];
	}

	void OP_ADD_I_Vx(instruction op)
	{
		const unsigned int Vx{ (op & 0x0F00u) >> 8u };
		index_ += registres_[Vx];
	}

	void OP_LD_F_Vx(instruction op)
	{
		const unsigned int Vx{ (op & 0x0F00u) >> 8u };
		index_ = 0x50 + (registres_[Vx] * 5);
	}

	void OP_LD_B_Vx(instruction op)
	{
		const unsigned int Vx{ (op & 0x0F00u) >> 8u };
		uint8_t valeur{ registres_[Vx] };
		memoire_[index_ + 2] = valeur % 10;
		valeur /= 10;
		memoire_[index_ + 1] = valeur % 10;
		valeur /= 10;
		memoire_[index_] = valeur % 10;
	}

	void OP_LD_I_Vx(instruction op)
	{
		const unsigned int Vx{ (op & 0x0F00u) >> 8u };
		for(octet i = 0; i <= Vx; ++i)
		{
			memoire_[index_ + i] = registres_[i];
		}
	}

	void OP_LD_Vx_I(instruction op)
	{
		const unsigned int Vx{ (op & 0x0F00u) >> 8u };
		for (octet i = 0; i <= Vx; ++i)
		{
			registres_[i] = memoire_[index_ + i];
		}
	}

protected:
	const static unsigned int V0 = 0;		const static unsigned int V8 = 8;
	const static unsigned int V1 = 1;		const static unsigned int V9 = 9;
	const static unsigned int V2 = 2;		const static unsigned int VA = 10;
	const static unsigned int V3 = 3;		const static unsigned int VB = 11;
	const static unsigned int V4 = 4;		const static unsigned int VC = 12;
	const static unsigned int V5 = 5;		const static unsigned int VD = 13;
	const static unsigned int V6 = 6;		const static unsigned int VE = 14;
	const static unsigned int V7 = 7;		const static unsigned int VF = 15;
	octet registres_[16]{ 0 };
	std::uint16_t index_{ 0 };

	adresse compteurProgramme_{ Processeur::DEBUT_ZONE_ACCESSIBLE };
	std::stack<adresse> pile_;

	octet chronoDelai_{ 0 };	// Delay timer
	octet chronoSon_{ 0 };	// Sound timer
	uint64_t compteurOps_{ 0 };

	IMemoire& memoire_;
	IVideoBuffer& video_;
	std::bitset<16> clavier_{ false };

	std::unordered_map< uint16_t, void (Processeur::*)(instruction op) > instructions
	{
		{ 0x00E0u, &Processeur::OP_CLS },
		{ 0x00EEu, &Processeur::OP_RET },
		{ 0x1000u, &Processeur::OP_JP },
		{ 0x2000u, &Processeur::OP_CALL },
		{ 0x3000u, &Processeur::OP_SE_Vx_Valeur },
		{ 0x4000u, &Processeur::OP_SNE_Vx_Valeur },
		{ 0x5000u, &Processeur::OP_SE_Vx_Vy },
		{ 0x6000u, &Processeur::OP_LD_Vx_Valeur },
		{ 0x7000u, &Processeur::OP_ADD_Vx_Valeur },
		{ 0x8000u, &Processeur::OP_LD_Vx_Vy },
		{ 0x8001u, &Processeur::OP_OR_Vx_Vy },
		{ 0x8002u, &Processeur::OP_AND_Vx_Vy },
		{ 0x8003u, &Processeur::OP_XOR_Vx_Vy },
		{ 0x8004u, &Processeur::OP_ADD_Vx_Vy },
		{ 0x8005u, &Processeur::OP_SUB_Vx_Vy },
		{ 0x8006u, &Processeur::OP_SHR_Vx },
		{ 0x8007u, &Processeur::OP_SUBN_Vx_Vy },
		{ 0x800Eu, &Processeur::OP_SHL_Vx },
		{ 0x9000u, &Processeur::OP_SNE_Vx_Vy },
		{ 0xA000u, &Processeur::OP_LD_I_Addr },
		{ 0xB000u, &Processeur::OP_JP_V0_Addr },
		{ 0xC000u, &Processeur::OP_RND_Vx_Valeur },
		{ 0xD000u, &Processeur::OP_DRW_Vx_Vy_Nibble },
		{ 0xE09Eu, &Processeur::OP_SKP_Vx },
		{ 0xE0A1u, &Processeur::OP_SKNP_Vx },
		{ 0xF007u, &Processeur::OP_LD_Vx_DT },
		{ 0xF00Au, &Processeur::OP_LD_Vx_K },
		{ 0xF015u, &Processeur::OP_LD_DT_Vx },
		{ 0xF018u, &Processeur::OP_LD_ST_Vx },
		{ 0xF01Eu, &Processeur::OP_ADD_I_Vx },
		{ 0xF029u, &Processeur::OP_LD_F_Vx },
		{ 0xF033u, &Processeur::OP_LD_B_Vx },
		{ 0xF055u, &Processeur::OP_LD_I_Vx },
		{ 0xF065u, &Processeur::OP_LD_Vx_I}
	};
};
