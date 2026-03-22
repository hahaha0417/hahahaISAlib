//---------------------------------------------------------------------------

#ifndef hahaha_emptyH
#define hahaha_emptyH
//---------------------------------------------------------------------------
#pragma once
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
namespace hahahaISAlib
{
//---------------------------------------------------------------------------

class hahaha_empty
{
public:
	hahaha_empty();
	~hahaha_empty();

	hahaha_empty(const hahaha_empty& he);
    hahaha_empty(hahaha_empty&& he) noexcept;
	hahaha_empty& operator=(const hahaha_empty& he);
	hahaha_empty& operator=(hahaha_empty&& he) noexcept;
	void Copy(const hahaha_empty& he);
    void Move(hahaha_empty&& he) noexcept;
public:
	int Reset();

public:

};



//---------------------------------------------------------------------------
} // hahahaISAlib
//---------------------------------------------------------------------------



#endif
