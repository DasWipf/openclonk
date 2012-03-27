/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2002, 2004-2006  Peter Wortmann
 * Copyright (c) 2001, 2005-2006  Sven Eberhardt
 * Copyright (c) 2006-2011  Günther Brammer
 * Copyright (c) 2007  Matthes Bender
 * Copyright (c) 2009  Nicolas Hake
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de
 *
 * Portions might be copyrighted by other authors who have contributed
 * to OpenClonk.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * See isc_license.txt for full license and disclaimer.
 *
 * "Clonk" is a registered trademark of Matthes Bender.
 * See clonk_trademark_license.txt for full license.
 */

#include <C4Include.h>
#include <C4Value.h>

#include <C4AulExec.h>
#include <C4DefList.h>
#include <C4StringTable.h>
#include <C4ValueArray.h>
#include <C4Game.h>
#include <C4GameObjects.h>
#include <C4Object.h>
#include <C4Log.h>

const C4Value C4VNull;

const char* GetC4VName(const C4V_Type Type)
{
	switch (Type)
	{
	case C4V_Nil:
		return "nil";
	case C4V_Int:
		return "int";
	case C4V_Float:
		return "float";
	case C4V_Bool:
		return "bool";
	case C4V_String:
		return "string";
	case C4V_Array:
		return "array";
	case C4V_PropList:
		return "proplist";
	case C4V_Numeric:
		return "any numeric";
	case C4V_Any:
		return "any";
	case C4V_Object:
		return "object";
	case C4V_Def:
		return "def";
	case C4V_Effect:
		return "effect";
	default:
		return "!Fehler!";
	}
}

C4Value::C4Value(C4Object *pObj): NextRef(NULL), Type(pObj ? C4V_PropList : C4V_Nil)
{
	Data.PropList = pObj; AddDataRef();
}

C4Object * C4Value::getObj() const
{
	return CheckConversion(C4V_Object) ? Data.PropList->GetObject() : NULL;
}

C4Object * C4Value::_getObj() const
{
	return Data.PropList ? Data.PropList->GetObject() : NULL;
}

C4Value C4VObj(C4Object *pObj) { return C4Value(static_cast<C4PropList*>(pObj)); }

bool C4Value::FnCnvObject() const
{
	// try casting
	if (Data.PropList->GetObject()) return true;
	return false;
}

bool C4Value::FnCnvDef() const
{
	// try casting
	if (Data.PropList->GetDef()) return true;
	return false;
}

bool C4Value::FnCnvEffect() const
{
	// try casting
	if (Data.PropList->GetEffect()) return true;
	return false;
}

bool C4Value::WarnAboutConversion(C4V_Type Type, C4V_Type vtToType)
{
	switch (vtToType)
	{
	case C4V_Nil:      return Type != C4V_Nil && Type != C4V_Any;
	case C4V_Float: case C4V_Numeric:
	case C4V_Int:      return Type != C4V_Int && Type != C4V_Float && Type != C4V_Numeric && Type != C4V_Nil && Type != C4V_Bool && Type != C4V_Any;
	case C4V_Bool:     return false;
	case C4V_PropList: return Type != C4V_PropList && Type != C4V_Effect && Type != C4V_Def && Type != C4V_Object && Type != C4V_Nil && Type != C4V_Any;
	case C4V_String:   return Type != C4V_String && Type != C4V_Nil && Type != C4V_Any;
	case C4V_Array:    return Type != C4V_Array && Type != C4V_Nil && Type != C4V_Any;
	case C4V_Function: return Type != C4V_Function && Type != C4V_Nil && Type != C4V_Any;
	case C4V_Any:      return false;
	case C4V_Def:      return Type != C4V_Def && Type != C4V_Object && Type != C4V_PropList && Type != C4V_Nil && Type != C4V_Any;
	case C4V_Object:   return Type != C4V_Object && Type != C4V_PropList && Type != C4V_Nil && Type != C4V_Any;
	case C4V_Effect:   return Type != C4V_Effect && Type != C4V_PropList && Type != C4V_Nil && Type != C4V_Any;
	default: assert(!"C4Value::ConvertTo: impossible conversion target"); return false;
	}
}

// Humanreadable debug output
StdStrBuf C4Value::GetDataString(int depth) const
{
	// ouput by type info
	switch (GetType())
	{
	case C4V_Int:
		return FormatString("%ld", static_cast<long>(_getInt()));
	case C4V_Float:
		return FormatString("%f", static_cast<float>(C4Real(Data.Float)));
	case C4V_Bool:
		return StdStrBuf(Data ? "true" : "false");
	case C4V_PropList:
	{
		if (Data.PropList == ScriptEngine.GetPropList())
			return StdStrBuf("Global");
		StdStrBuf DataString;
		DataString = "{";
		if (Data.PropList->GetObject())
		{
			if (Data.PropList->GetObject()->Status == C4OS_NORMAL)
				DataString.AppendFormat("#%d, ", Data.PropList->GetObject()->Number);
			else
				DataString.AppendFormat("(#%d), ", Data.PropList->GetObject()->Number);
		}
		else if (Data.PropList->GetDef())
			DataString.AppendFormat("%s, ", Data.PropList->GetDef()->id.ToString());
		Data.PropList->AppendDataString(&DataString, ", ", depth);
		DataString.AppendChar('}');
		return DataString;
	}
	case C4V_String:
		return (Data.Str && Data.Str->GetCStr()) ? FormatString("\"%s\"", Data.Str->GetCStr()) : StdStrBuf("(nullstring)");
	case C4V_Array:
	{
		if (depth <= 0 && Data.Array->GetSize())
		{
			return StdStrBuf("[...]");
		}
		StdStrBuf DataString;
		DataString = "[";
		for (int32_t i = 0; i < Data.Array->GetSize(); i++)
		{
			if (i) DataString.Append(", ");
			DataString.Append(std::move(Data.Array->GetItem(i).GetDataString(depth - 1)));
		}
		DataString.AppendChar(']');
		return DataString;
	}
	case C4V_Function:
		return Data.Fn->GetFullName();
	case C4V_Nil:
		return StdStrBuf("nil");
	default:
		return StdStrBuf("-unknown type- ");
	}
}

C4Value C4VString(const char *strString)
{
	// safety
	if (!strString) return C4Value();
	return C4Value(::Strings.RegString(strString));
}

C4Value C4VString(StdStrBuf Str)
{
	// safety
	if (Str.isNull()) return C4Value();
	return C4Value(::Strings.RegString(Str));
}

const C4Value & C4ValueNumbers::GetValue(uint32_t n)
{
	if (n <= LoadedValues.size())
		return LoadedValues[n - 1];
	LogF("ERROR: Value number %d is missing.", n);
	return C4VNull;
}

void C4Value::Denumerate(class C4ValueNumbers * numbers)
{
	switch (Type)
	{
	case C4V_Enum:
		Set(numbers->GetValue(Data.Int)); break;
	case C4V_Array:
		Data.Array->Denumerate(numbers); break;
	case C4V_PropList:
		// objects and effects are denumerated via the main object list
		if (!Data.PropList->IsNumbered() && !Data.PropList->IsDef())
			Data.PropList->Denumerate(numbers);
		break;
	case C4V_C4ObjectEnum:
		{
			C4PropList *pObj = C4PropListNumbered::GetByNumber(Data.Int);
			if (pObj)
				// set
				SetPropList(pObj);
			else
			{
				// object: invalid value - set to zero
				LogF("ERROR: Object number %d is missing.", int(Data.Int));
				Set0();
			}
		}
	default: break;
	}
}

void C4ValueNumbers::Denumerate()
{
	for (std::vector<C4Value>::iterator i = LoadedValues.begin(); i != LoadedValues.end(); ++i)
		i->Denumerate(this);
}

uint32_t C4ValueNumbers::GetNumberForValue(C4Value * v)
{
	// This is only used for C4Values containing pointers
	// Assume that all pointers have the same size
	if (ValueNumbers.find(v->GetData()) == ValueNumbers.end())
	{
		ValuesToSave.push_back(v);
		ValueNumbers[v->GetData()] = ValuesToSave.size();
		return ValuesToSave.size();
	}
	return ValueNumbers[v->GetData()];
}

void C4Value::CompileFunc(StdCompiler *pComp, C4ValueNumbers * numbers)
{
	// Type
	bool fCompiler = pComp->isCompiler();
	char cC4VID;
	if (!fCompiler)
	{
		assert(Type != C4V_Nil || !Data);
		switch (Type)
		{
		case C4V_Nil:
			cC4VID = 'n'; break;
		case C4V_Int:
			cC4VID = 'i'; break;
		case C4V_Bool:
			cC4VID = 'b'; break;
		case C4V_PropList:
			if (getPropList()->IsDef())
				cC4VID = 'D';
			else if (getPropList()->IsNumbered())
				cC4VID = 'O';
			else if (getPropList() == GameScript.ScenPropList)
				cC4VID = 'c';
			else if (getPropList() == GameScript.ScenPrototype)
				cC4VID = 't';
			else if (getPropList() == ScriptEngine.GetPropList())
				cC4VID = 'g';
			else
				cC4VID = 'E';
			break;
		case C4V_Array:
			cC4VID = 'E'; break;
		case C4V_Function:
			cC4VID = 'f'; break;
		case C4V_String:
			cC4VID = 's'; break;
		default:
			assert(false);
		}
	}
	pComp->Character(cC4VID);
	// Data
	int32_t iTmp;
	switch (cC4VID)
	{
	case 'i':
		iTmp = Data.Int;
		pComp->Value(iTmp);
		SetInt(iTmp);
		break;
	case C4V_Float:
		{
			C4Real val = Data.Float;
			pComp->Value(val);
			Data.Float = val;
		}

	case 'b':
		iTmp = Data.Int;
		pComp->Value(iTmp);
		SetBool(iTmp);
		break;

	case 'E':
		if (!fCompiler)
			iTmp = numbers->GetNumberForValue(this);
		pComp->Value(iTmp);
		if (fCompiler)
		{
			Data.Int = iTmp; // must be denumerated later
			Type = C4V_Enum;
		}
		break;

	case 'O':
		if (!fCompiler)
			iTmp = getPropList()->GetPropListNumbered()->Number;
		pComp->Value(iTmp);
		if (fCompiler)
		{
			Data.Int = iTmp; // must be denumerated later
			Type = C4V_C4ObjectEnum;
		}
		break;

	case 'D':
	{
		C4ID id;
		if (!fCompiler)
			id = getPropList()->GetDef()->id;
		pComp->Value(id);
		if (fCompiler)
		{
			C4PropList * p = Definitions.ID2Def(id);
			if (!p)
			{
				Set0();
				pComp->Warn("ERROR: Definition %s is missing.", id.ToString());
			}
			else
			{
				SetPropList(p);
			}
		}
		break;
	}

	case 's':
	{
		StdStrBuf s;
		if (!fCompiler)
			s = Data.Str->GetData();
		pComp->Value(s);
		if (fCompiler)
			SetString(::Strings.RegString(s));
		break;
	}

	case 'c':
		if (fCompiler)
			SetPropList(GameScript.ScenPropList);
		break;

	case 't':
		if (fCompiler)
			SetPropList(GameScript.ScenPrototype);
		break;

	case 'g':
		if (fCompiler)
			SetPropList(ScriptEngine.GetPropList());
		break;

	case 'n':
	case 'A': // compat with OC 5.1
		if (fCompiler)
			Set0();
		// doesn't have a value, so nothing to store
		break;

	case 'f':
	{
		C4Value Owner;
		if (!fCompiler)
		{
			Owner.SetPropList(Data.Fn->Owner->GetPropList());
			assert(Owner._getPropList() && Owner._getPropList()->GetFunc(Data.Fn->GetName()) == Data.Fn);
		}
		pComp->Value(mkParAdapt(Owner, numbers));
		pComp->Separator(StdCompiler::SEP_PART);
		StdStrBuf s;
		if (!fCompiler)
			s = Data.Fn->GetName();
		pComp->Value(s);
		if (fCompiler)
		{
			// Owner was a definition or singleton, so shouldn't have to be denumerated
			if (!Owner.getPropList())
			{
				Set0();
				pComp->Warn("ERROR: Owner of function %s is missing.", s.getData());
			}
			else
				SetFunction(Owner._getPropList()->GetFunc(s.getData()));
		}
		break;
	}

	default:
		// shouldn't happen
		pComp->excCorrupt("unknown C4Value type tag '%c'", cC4VID);
		break;
	}
}

void C4ValueNumbers::CompileValue(StdCompiler * pComp, C4Value * v)
{
	// Type
	bool fCompiler = pComp->isCompiler();
	char cC4VID;
	switch(v->GetType())
	{
	case C4V_PropList: cC4VID = 'p'; break;
	case C4V_Array:    cC4VID = 'a'; break;
	default: assert(fCompiler); break;
	}
	pComp->Character(cC4VID);
	pComp->Separator(StdCompiler::SEP_START);
	switch(cC4VID)
	{
	case 'p':
		{
			C4PropList * p = v->_getPropList();
			pComp->Value(mkParAdapt(mkPtrAdaptNoNull(p), this));
			if (fCompiler) v->SetPropList(p);
		}
		break;
	case 'a':
		{
			C4ValueArray * a = v->_getArray();
			pComp->Value(mkParAdapt(mkPtrAdaptNoNull(a), this));
			if (fCompiler) v->SetArray(a);
		}
		break;
	default:
		pComp->excCorrupt("Unexpected character '%c'", cC4VID);
		break;
	}
	pComp->Separator(StdCompiler::SEP_END);
}

void C4ValueNumbers::CompileFunc(StdCompiler * pComp)
{
	bool fCompiler = pComp->isCompiler();
	bool fNaming = pComp->hasNaming();
	if (fCompiler)
	{
		uint32_t iSize;
		if (!fNaming) pComp->Value(iSize);
		// Read new
		do
		{
			// No entries left to read?
			if (!fNaming && !iSize--)
				break;
			// Read entries
			try
			{
				LoadedValues.push_back(C4Value());
				CompileValue(pComp, &LoadedValues.back());
			}
			catch (StdCompiler::NotFoundException *pEx)
			{
				// No value found: Stop reading loop
				delete pEx;
				break;
			}
		}
		while (pComp->Separator(StdCompiler::SEP_SEP));
	}
	else
	{
		// Note: the list grows during this loop due to nested data structures.
		// Data structures with loops are fine because the beginning of the loop
		// will be found in the map and not saved again.
		// This may still work with the binary compilers due to double-compiling
		if (!fNaming)
		{
			int32_t iSize = ValuesToSave.size();
			pComp->Value(iSize);
		}
		for(std::list<C4Value *>::iterator i = ValuesToSave.begin(); i != ValuesToSave.end(); ++i)
		{
			CompileValue(pComp, *i);
			if (i != ValuesToSave.end()) pComp->Separator(StdCompiler::SEP_SEP);
		}
	}
}

bool C4Value::operator == (const C4Value& Value2) const
{
	switch (Type)
	{
	case C4V_Any:
		assert(!Data);
		return Value2.Type == Type;
	case C4V_Int:
	case C4V_Bool:
		switch (Value2.Type)
		{
		case C4V_Int:
		case C4V_Bool:
			return Data == Value2.Data;
		case C4V_Float:
			return C4Real(Value2.Data.Float) == static_cast<int>(Data.Int);
		default:
			return false;
		}
	case C4V_Float:
		switch (Value2.Type)
		{
		case C4V_Int:
		case C4V_Bool:
			return C4Real(Data.Float) == static_cast<int>(Value2.Data.Int);
		case C4V_Float:
			return Data.Float == Value2.Data.Float;
		default:
			return false;
		}
	case C4V_PropList:
		if (Value2.Type == C4V_PropList)
		{
			// Compare as equal if and only if the proplists are indistinguishable
			// If one or both are mutable, they have to be the same
			// otherwise, they have to have the same contents
			if (Data.PropList == Value2.Data.PropList) return true;
			if (!Data.PropList->IsFrozen() || !Value2.Data.PropList->IsFrozen()) return false;
			return (*Data.PropList == *Value2.Data.PropList);
		}
	case C4V_String:
		return Type == Value2.Type && Data.Str == Value2.Data.Str;
	case C4V_Array:
		return Type == Value2.Type && *(Data.Array) == *(Value2.Data.Array);
	default:
		return Data == Value2.Data;
	}
	return GetData() == Value2.GetData();
}

bool C4Value::operator != (const C4Value& Value2) const
{
	return !(*this == Value2);
}

C4ID C4Value::getC4ID() const
{
	C4PropList * p = getPropList();
	if (!p) return C4ID::None;
	C4Def * d = p->GetDef();
	if (!d) return C4ID::None;
	return d->id;
}

void C4Value::LogDeletedObjectWarning(C4PropList * p)
{
	if (p->GetPropListNumbered())
		LogF("Warning: using deleted object (#%d) (%s)!", p->GetPropListNumbered()->Number, p->GetName());
	else
		LogF("Warning: using deleted proplist %p (%s)!", static_cast<void*>(p), p->GetName());
	AulExec.LogCallStack();
}
