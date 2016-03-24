/**
	Producer System
	Unit tests for the producers system. Invokes tests by calling the 
	global function Test*_OnStart(int plr) and iterate through all 
	tests. The test is completed once Test*_Completed() returns
	true. Then Test*_OnFinished() is called, to be able to reset 
	the scenario for the next test.
	
	With LaunchTest(int nr) a specific test can be launched when
	called during runtime. A test can be skipped by calling the
	function SkipTest().
	
	@author Maikel (unit test logic), Marky (tests)
*/

static const EXPECTED_TESTS = 14;

protected func Initialize()
{
	// Energy is not our concern in this test
	CreateObject(Rule_NoPowerNeed);
	return;
}

protected func InitializePlayer(int plr)
{
	// Set zoom to full map size.
	SetPlayerZoomByViewRange(plr, LandscapeWidth(), nil, PLRZOOM_Direct);
	
	// No FoW to see everything happening.
	SetFoW(false, plr);
	
	// All players belong to the first team.
	// The second team only exists for testing.
	SetPlayerTeam(plr, 1);
		
	// Move player to the start of the scenario.
	GetCrew(plr)->SetPosition(120, 150);
	
	// Some knowledge to construct a flagpole.
	GetCrew(plr)->CreateContents(Hammer);
	SetPlrKnowledge(plr, Flagpole);
	
	// Add test control effect.
	var effect = AddEffect("IntTestControl", nil, 100, 2);
	effect.testnr = 1;
	effect.launched = false;
	effect.plr = plr;
	return;
}


/*-- Test Control --*/

// Aborts the current test and launches the specified test instead.
global func LaunchTest(int nr)
{
	// Get the control effect.
	var effect = GetEffect("IntTestControl", nil);
	if (!effect)
	{
		// Create a new control effect and launch the test.
		effect = AddEffect("IntTestControl", nil, 100, 2);
		effect.testnr = nr;
		effect.launched = false;
		effect.plr = GetPlayerByIndex(0, C4PT_User);
		return;
	}
	// Finish the currently running test.
	Call(Format("~Test%d_OnFinished", effect.testnr));
	// Start the requested test by just setting the test number and setting 
	// effect.launched to false, effect will handle the rest.
	effect.testnr = nr;
	effect.launched = false;
	return;
}

// Calling this function skips the current test, does not work if last test has been ran already.
global func SkipTest()
{
	// Get the control effect.
	var effect = GetEffect("IntTestControl", nil);
	if (!effect)
		return;
	// Finish the previous test.
	Call(Format("~Test%d_OnFinished", effect.testnr));
	// Start the next test by just increasing the test number and setting 
	// effect.launched to false, effect will handle the rest.
	effect.testnr++;
	effect.launched = false;
	return;
}


/*-- Test Effect --*/

global func FxIntTestControlStart(object target, proplist effect, int temporary)
{
	if (temporary)
		return FX_OK;
	// Set default interval.
	effect.Interval = 2;
	return FX_OK;
}

global func FxIntTestControlTimer(object target, proplist effect)
{
	// Launch new test if needed.
	if (!effect.launched)
	{
		// Log test start.
		Log("=====================================");
		Log("Test %d started:", effect.testnr);
		// Start the test if available, otherwise finish test sequence.
		if (!Call(Format("~Test%d_OnStart", effect.testnr), effect.plr))
		{
			Log("Test %d not available, the previous test was the last test.", effect.testnr);
			Log("=====================================");
			effect.testnr--;
			if (effect.testnr == EXPECTED_TESTS)
				Log("All tests have been successfully completed!");
			else
				Log("Executed %d of %d tests.", effect.testnr, EXPECTED_TESTS);
			return -1;
		}
		effect.launched = true;
	}		
	// Check whether the current test has been finished.
	if (Call(Format("Test%d_Completed", effect.testnr)))
	{
		effect.launched = false;
		//RemoveTest();
		// Call the test on finished function.
		Call(Format("~Test%d_OnFinished", effect.testnr));
		// Log result and increase test number.
		Log("Test %d successfully completed.", effect.testnr);
		effect.testnr++;
	}
	return FX_OK;
}


/*-- Producer Tests --*/

global func Test1_OnStart(int plr)
{
	var passed = true;
	var producer = CreateObject(Foundry);
	
	Log("Test behaviour of AddToQueue(), ClearQueue() and GetQueue()");
	
	passed &= doTest("The queue should be empty on initialization. Got %v, expected %v.", GetLength(producer->GetQueue()), 0);

	Log("****** Adding items to the queue");

	producer->AddToQueue(GoldBar, 2);
	passed &= doTest("The queue gets filled when adding things to the queue. Got %d, expected %d.", GetLength(producer->GetQueue()), 1);
	if (GetLength(producer->GetQueue()) > 0)
		passed &= doTestQueueEntry(producer->GetQueue()[0], GoldBar, 2, false);
	
	Log("****** Queueing the same product again increases the existing queue");
	
	producer->AddToQueue(GoldBar, 2);
	passed &= doTest("Entries stay the same. Got %d, expected %d.", GetLength(producer->GetQueue()), 1);
	if (GetLength(producer->GetQueue()) > 0)
		passed &= doTestQueueEntry(producer->GetQueue()[0], GoldBar, 4, false);
	
	Log("****** Queueing the same product again with an infinite count increases the existing queue.");
	
	producer->AddToQueue(GoldBar, 2, true);
	passed &= doTest("Entries stay the same. Got %d, expected %d.", GetLength(producer->GetQueue()), 1);
	if (GetLength(producer->GetQueue()) > 0)
		passed &= doTestQueueEntry(producer->GetQueue()[0], GoldBar, 6, true);

	Log("****** Queueing the same product without onto an existing infinite product in the queue");

	producer->AddToQueue(GoldBar, 2, false);
	passed &= doTest("Entries stay the same. Got %d, expected %d.", GetLength(producer->GetQueue()), 1);
	if (GetLength(producer->GetQueue()) > 0)
		passed &= doTestQueueEntry(producer->GetQueue()[0], GoldBar, 8, false);
	
	Log("****** Adding more items to the queue, next item should be added to the end.");

	producer->AddToQueue(Loam, 4, false);
	passed &= doTest("Entries increase. Got %d, expected %d.", GetLength(producer->GetQueue()), 2);
	if (GetLength(producer->GetQueue()) > 1)
	{
		passed &= doTestQueueEntry(producer->GetQueue()[0], GoldBar, 8, false);
		passed &= doTestQueueEntry(producer->GetQueue()[1], Loam, 4, false);
	}
	
	Log("****** Adding more infinite items to the queue, next item should be added to the end.");

	producer->AddToQueue(Metal, 0, true);
	passed &= doTest("Entries increase. Got %d, expected %d.", GetLength(producer->GetQueue()), 3);
	if (GetLength(producer->GetQueue()) > 2)
	{
		passed &= doTestQueueEntry(producer->GetQueue()[0], GoldBar, 8, false);
		passed &= doTestQueueEntry(producer->GetQueue()[1], Loam, 4, false);
		passed &= doTestQueueEntry(producer->GetQueue()[2], Metal, 0, true);
	}
	
	Log("****** Clearing the queue");
	
	producer->ClearQueue();
	passed &= doTest("Queue is empty. Got %d entries, expected %d.", GetLength(producer->GetQueue()), 0);

	Log("****** Adding a product with amount 0 to the queue");

	producer->AddToQueue(GoldBar, 0);
	passed &= doTest("The queue gets filled. Got %d, expected %d.", GetLength(producer->GetQueue()), 1);
	if (GetLength(producer->GetQueue()) > 0)
		passed &= doTestQueueEntry(producer->GetQueue()[0], GoldBar, 0, false);

	producer->RemoveObject();

	return passed;
}
global func Test1_Completed(){	return true; }
global func Test1_OnFinished(){	return; }


global func Test2_OnStart(int plr)
{
	var passed = true;
	var producer = CreateObject(Foundry);

	var amount_gold = 123, amount_loam = 456, amount_metal = 789;

	producer->AddToQueue(GoldBar, amount_gold);
	producer->AddToQueue(Loam, amount_loam);
	producer->AddToQueue(Metal, amount_metal);
	
	Log("Testing the behaviour of CycleQueue(), GetQueueIndex(), ModifyQueueIndex()");
	
	passed &= doTestQueueEntry(producer->GetQueue()[0], GoldBar, amount_gold, false);
	passed &= doTestQueueEntry(producer->GetQueue()[1], Loam, amount_loam, false);
	passed &= doTestQueueEntry(producer->GetQueue()[2], Metal, amount_metal, false);
	
	Log("****** GetQueueIndex()");
	
	passed &= doTest("The product 'GoldBar' has the correct index. Got %d, expected %d.", producer->GetQueueIndex(GoldBar), 0);
	passed &= doTest("The product 'Loam' has the correct index. Got %d, expected %d.", producer->GetQueueIndex(Loam), 1);
	passed &= doTest("The product 'Metal' has the correct index. Got %d, expected %d.", producer->GetQueueIndex(Metal), 2);
	passed &= doTest("The product 'Barrel' is not in the queue. Got %d, expected %d.", producer->GetQueueIndex(Barrel), nil);

	Log("****** CycleQueue(), should move the first item to the end");
	
	producer->CycleQueue();
	passed &= doTest("The product 'GoldBar' has the correct index. Got %d, expected %d.", producer->GetQueueIndex(GoldBar), 2);
	passed &= doTest("The product 'Loam' has the correct index. Got %d, expected %d.", producer->GetQueueIndex(Loam), 0);
	passed &= doTest("The product 'Metal' has the correct index. Got %d, expected %d.", producer->GetQueueIndex(Metal), 1);

	Log("****** CycleQueue()");
	
	producer->CycleQueue();
	passed &= doTest("The product 'GoldBar' has the correct index. Got %d, expected %d.", producer->GetQueueIndex(GoldBar), 1);
	passed &= doTest("The product 'Loam' has the correct index. Got %d, expected %d.", producer->GetQueueIndex(Loam), 2);
	passed &= doTest("The product 'Metal' has the correct index. Got %d, expected %d.", producer->GetQueueIndex(Metal), 0);

	Log("****** CycleQueue(), queue should be in the original order again");
	
	producer->CycleQueue();
	passed &= doTest("The product 'GoldBar' has the correct index. Got %d, expected %d.", producer->GetQueueIndex(GoldBar), 0);
	passed &= doTest("The product 'Loam' has the correct index. Got %d, expected %d.", producer->GetQueueIndex(Loam), 1);
	passed &= doTest("The product 'Metal' has the correct index. Got %d, expected %d.", producer->GetQueueIndex(Metal), 2);

	Log("Testing ModifyQueueIndex()");
	
	Log("****** Modify amount +1");
	
	producer->ModifyQueueIndex(0, +1, nil);
	passed &= doTestQueueEntry(producer->GetQueue()[0], GoldBar, amount_gold + 1, false);

	Log("****** Modify amount -2");

	producer->ModifyQueueIndex(0, -2, nil);
	passed &= doTestQueueEntry(producer->GetQueue()[0], GoldBar, amount_gold - 1, false);

	Log("****** Set infinity to true");

	producer->ModifyQueueIndex(0, nil, true);
	passed &= doTestQueueEntry(producer->GetQueue()[0], GoldBar, amount_gold - 1, true);

	Log("****** Modify amount so that it is -1, while infinite");

	producer->ModifyQueueIndex(0, -amount_gold, nil);
	passed &= doTestQueueEntry(producer->GetQueue()[0], GoldBar, -1, true);

	Log("****** Modify infinity to false, product should be removed from the queue");

	producer->ModifyQueueIndex(0, nil, false);
	passed &= doTestQueueEntry(producer->GetQueue()[0], Loam, amount_loam, false);
	passed &= doTestQueueEntry(producer->GetQueue()[1], Metal, amount_metal, false);

	Log("****** Modify amount of item so that it is 0, while finite");

	producer->ModifyQueueIndex(0, - amount_loam, nil);
	passed &= doTestQueueEntry(producer->GetQueue()[0], Metal, amount_metal, false);

	producer->RemoveObject();
	return passed;
}
global func Test2_Completed(){	return true; }
global func Test2_OnFinished(){	return; }

// CheckComponent
// GetAvailableComponentAmount
global func Test3_OnStart(int plr)
{
	var passed = true;
	return passed;
}
global func Test3_Completed(){	return true; }
global func Test3_OnFinished(){	return; }


// CheckFuel
global func Test4_OnStart(int plr)
{
	var passed = true;
	return passed;
}
global func Test4_Completed(){	return true; }
global func Test4_OnFinished(){	return; }

// ProductionCosts
global func Test5_OnStart(int plr)
{
	var passed = true;
	return passed;
}
global func Test5_Completed(){	return true; }
global func Test5_OnFinished(){	return; }

// IsCollectionAllowed
global func Test6_OnStart(int plr)
{
	var passed = true;
	return passed;
}
global func Test6_Completed(){	return true; }
global func Test6_OnFinished(){	return; }


// Producer with liquid need and pseudo liquid object.
global func Test7_OnStart(int plr)
{
	// Producer: Foundry
	var passed = true;
	var producer = CreateObjectAbove(Foundry, 75, 160, plr);
	producer->CreateContents(Earth, 10);

	// Log what the test is about.
	Log("Objects with liquid need (loam), can be produced with pseudo liquid objects (ice)");

	var ice1, ice2;
	ice1 = CreateObject(Ice);
	ice2 = CreateObject(Ice);
	producer->Collect(ice1, true);
	producer->Collect(ice2, true);
	passed &= doTest("Producer contains no ice chunks. Got %d, expected %d.", producer->ContentsCount(Ice), 0);
	passed &= doTest("Producer contains no water. Got %d, expected %d.", producer->ContentsCount(Liquid_Water), 0);

	ice1->Enter(producer);
	ice2->Enter(producer);
	passed &= doTest("Producer contains ice chunks when forced. Got %d, expected %d.", producer->ContentsCount(Ice), 2);


	producer->AddToQueue(Loam, 5); // needs 300 water
	passed &= doTest("Producer contains ice chunks. Got %d, expected %d.", producer->ContentsCount(Ice), 2);
	passed &= doTest("Producer contains no water. Got %d, expected %d.", producer->ContentsCount(Liquid_Water), 0);

	ice1 = CreateObject(Ice);
	ice2 = CreateObject(Ice);
	producer->Collect(ice1, true);
	producer->Collect(ice2, true);

	passed &= doTest("Producer contains ice chunks. Got %d, expected %d.", producer->ContentsCount(Ice), 2);
	passed &= doTest("Producer contains water. Got %d, expected %d.", producer->FindContents(Liquid_Water)->GetLiquidAmount(), 400);

	return passed;
}

global func Test7_Completed()
{
	SetTemperature(-10);
	if (ObjectCount(Find_ID(Loam)) >= 5 // the loam was created
	 && ObjectCount(Find_ID(Ice)) == 2 // and exactly the two ice objects that were in the producer before stayed there
	 && (FindObject(Find_ID(Liquid_Water))->GetLiquidAmount() == 100)) // not all water was used up
		return true;
	return false;
}

global func Test7_OnFinished()
{
	// Remove the created objects
	RemoveAll(Find_Or(Find_ID(Foundry), Find_ID(Loam)));
	return;
}

// Producer with liquid need and liquid container.
global func Test8_OnStart(int plr)
{
	// Producer: Foundry
	var producer = CreateObjectAbove(Foundry, 75, 160, plr);
	producer->CreateContents(Earth, 10);
	var barrel = CreateObject(Barrel);
	barrel->PutLiquid("Water", 300); // contains 300 water
	producer->AddToQueue(Loam, 5); // needs 300 water
	producer->Collect(barrel, true);

	// Log what the test is about.
	Log("Objects with liquid need (loam), can be produced with liquid containers (barrel). The liquid container must not be removed.");
	return true;
}

global func Test8_Completed()
{
    // The barrel must not be removed.
	if (ObjectCount(Find_ID(Loam)) >= 5 && ObjectCount(Find_ID(Barrel)) >= 1)
		return true;
	return false;
}

global func Test8_OnFinished()
{
	// Remove wind generator, compensator and workshop.
	RemoveAll(Find_Or(Find_ID(Foundry), Find_ID(Loam), Find_ID(Barrel)));
	return;
}


// Producer with liquid need and liquid object.
global func Test9_OnStart(int plr)
{
	// Producer: Foundry
	var producer = CreateObjectAbove(Foundry, 75, 160, plr);
	producer->CreateContents(Earth, 10);
	var water = CreateObject(Liquid_Water);
	water->SetStackCount(400);
	producer->AddToQueue(Loam, 5); // needs 300 water
	water->Enter(producer);

	// Log what the test is about.
	Log("Objects with liquid need (loam), can be produced with liquid containers (barrel). The liquid container must not be removed.");
	return true;
}

global func Test9_Completed()
{
    // The liquid must not be removed.
	if (ObjectCount(Find_ID(Loam)) >= 5 && ObjectCount(Find_ID(Liquid_Water)) >= 1)
		return true;
	return false;
}

global func Test9_OnFinished()
{
	RemoveAll(Find_Or(Find_ID(Foundry), Find_ID(Loam), Find_ID(Liquid_Water)));
	return;
}


// Producer with fuel need, fuel object
global func Test10_OnStart(int plr)
{
	// Producer: Foundry
	var producer = CreateObjectAbove(Foundry, 75, 160, plr);
	producer->CreateContents(Wood, 10);
	producer->CreateContents(Ore, 5);
	producer->AddToQueue(Metal, 5);

	// Log what the test is about.
	Log("Objects with fuel need (metal), can be produced with fuel objects (wood). The fuel is used up.");
	return true;
}

global func Test10_Completed()
{
	if (ObjectCount(Find_ID(Metal)) >= 5 && ObjectCount(Find_ID(Wood)) <= 0)
		return true;
	return false;
}

global func Test10_OnFinished()
{
	RemoveAll(Find_Or(Find_ID(Foundry), Find_ID(Metal)));
	return;
}


// Producer with fuel need, liquid container
global func Test11_OnStart(int plr)
{
	// Producer: Foundry
	var producer = CreateObjectAbove(Foundry, 75, 160, plr);
	producer->CreateContents(Ore, 5);
	producer->AddToQueue(Metal, 5);
	
	var barrelA = CreateObject(Barrel);
	var barrelB = CreateObject(Barrel);
	barrelA->PutLiquid("Oil", 300);
	barrelB->PutLiquid("Oil", 300);

	producer->Collect(barrelA, true);
	producer->Collect(barrelB, true);

	// Log what the test is about.
	Log("Objects with fuel need (metal), can be produced with fuel from a liquid container (barrel with oil). The fuel is used up.");
	return true;
}

global func Test11_Completed()
{
	if (ObjectCount(Find_ID(Metal)) >= 5 // metal is produced
	 && ObjectCount(Find_ID(Barrel)) == 2 // barrels stay
	 && FindObject(Find_ID(Liquid_Oil)) // oil remains
	 && FindObject(Find_ID(Liquid_Oil))->GetLiquidAmount() == 100) // the correct amount was used
		return true;
	return false;
}

global func Test11_OnFinished()
{
	RemoveAll(Find_Or(Find_ID(Foundry), Find_ID(Metal), Find_ID(Barrel), Find_ID(Liquid_Oil)));
	return;
}


// Fuel functions behave correclty
global func Test12_OnStart(int plr)
{
	// Log what the test is about.
	Log("Objects that are fuel should return a value > 0 if no parameter is passed to GetFuelAmount().");
	// Get the control effect.
	var effect = GetEffect("IntTestControl", nil);
	if (!effect) return false;

	var passed = true;
	var def;
	for (var i = 0; def = GetDefinition(i); ++i)
	if (def->~IsFuel())
	{
		var instance = CreateObject(def);
		var amount = instance->~GetFuelAmount();
		Log("Default fuel amount in %i: %d > 0?", def, amount);
		passed &= (amount > 0);
		if (instance) instance->RemoveObject();
	}
	
	effect.passed_test_6 = passed;

	return true;
}

global func Test12_Completed()
{
	return GetEffect("IntTestControl", nil).passed_test_6;
}

global func Test12_OnFinished()
{
	return;
}


// Producer stops production on insufficient material
global func Test13_OnStart(int plr)
{
	var producer = CreateObjectAbove(ToolsWorkshop, 75, 160, plr);
	producer->CreateContents(Wood, 10);
	producer->CreateContents(Metal, 1);
	producer->AddToQueue(Barrel, 5);

	// Log what the test is about.
	Log("Producer halts production if materials are insufficient, continues when new material comes in.");

	GetEffect("IntTestControl", nil).timer = 0;
	return true;
}

global func Test13_Completed()
{
	var fx = GetEffect("IntTestControl", nil);
	fx.timer++;

	if (fx.timer < 50) return false; // it would take this long to produce 5 barrels

	if (ObjectCount(Find_ID(Barrel)) == 1
	 && ObjectCount(Find_ID(Wood)) == 8) return true;
	
	return false;
}

global func Test13_OnFinished()
{
	RemoveAll(Find_Or(Find_ID(ToolsWorkshop), Find_ID(Barrel)));
	return;
}



// Producer stops production on insufficient material
global func Test14_OnStart(int plr)
{
	var producer = CreateObjectAbove(ToolsWorkshop, 75, 160, plr);
	producer->CreateContents(Wood, 10);
	producer->CreateContents(Metal, 1);
	producer->AddToQueue(Barrel, 5);

	// Log what the test is about.
	Log("Producer halts production if materials are insufficient, continues when new material comes in.");

	GetEffect("IntTestControl", nil).timer = 0;
	return true;
}

global func Test14_Completed()
{
	if (ObjectCount(Find_ID(Barrel)) >= 5) return true;

	var fx = GetEffect("IntTestControl", nil);
	fx.timer++;
	
	if (fx.timer >= 120)
	{
		FindObject(Find_ID(ToolsWorkshop))->CreateContents(Metal);
		fx.timer = 100;
	}
	
	return false;
}

global func Test14_OnFinished()
{
	RemoveAll(Find_Or(Find_ID(ToolsWorkshop), Find_ID(Barrel), Find_ID(Wood)));
	return;
}


/*-- Helper Functions --*/

global func doTest(description, returned, expected)
{
	var test = (returned == expected);
	
	var predicate = "[Fail]";
	if (test) predicate = "[Pass]";
	
	Log(Format("%s %s", predicate, description), returned, expected);
	return test;
}

global func doTestQueueEntry(proplist entry, id product, int amount, bool infinite)
{
	var passed = true;
	passed &= doTest("The queued product is as expected. Got %i, expected %i.", entry.Product, product);
	passed &= doTest("The queued amount is as expected. Got %d, expected %d.", entry.Amount, amount);
	passed &= doTest("The queued infinity setting is as expected. Got %v, expected %v.", entry.Infinite ?? false, infinite);
	return passed;
}
