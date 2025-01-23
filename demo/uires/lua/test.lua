win = nil;
tid = 0;
gamewnd = nil;
gamecanvas = nil;
players = {};
flag_win = nil;
runTimer = nil;

coins_all = 100;	--现有资金
coins_bet = {0,0,0,0} --下注金额
bet_rate = 4;		--赔率
prog_max	 = 200;	--最大步数
prog_all = {0,0,0,0} --马匹进度

function on_host_msg(hostWnd,msg, wp,lp,pRes)
	--slog("test on host msg:" .. msg);
	if msg == 0x82 then -- 0x82 == WM_NCDESTROY
		HostWnd_SetMsgHandler(hostWnd,"",nil);
	end
	return 0;
end

function onBtnLrc(e)
 slog("btn lrc clicked!");
 local btn = toSWindow(e:Sender());
 local hwnd = btn:GetHostHwnd();
 local ret = SMessageBox(hwnd,L"onBtnLrc(我是中文), cancel origin proc?",L"test",1);
 if ret == 1 then
	 e:SetBubbleUp(0); --block origin behavior.
 end
 return 1;
end

function on_init(args)
	--初始化全局对象
	slog("on_init");
	win = getHostFromInitEvent(args);
	HostWnd_SetMsgHandler(win,"on_host_msg",win);

	if win == nil then
		return 0;
	end
	local root = toSWindow(args:Sender());
	gamewnd = root:FindChildByNameA("game_wnd",-1);
	gamecanvas = gamewnd:FindChildByNameA("game_canvas",-1);
	flag_win = gamewnd:FindChildByNameA("flag_win",-1);
	players = {
					gamecanvas:FindChildByNameA("player_1",-1),
					gamecanvas:FindChildByNameA("player_2",-1),
					gamecanvas:FindChildByNameA("player_3",-1),
					gamecanvas:FindChildByNameA("player_4",-1)
				    };
	--布局
	on_canvas_size(nil);

	--show how to do SubscribeEvent
	local btnLrc = root:FindChildByNameA("btn_lrc",-1);
	local lrcSlot = CreateEventSlot("onBtnLrc");
	btnLrc:SubscribeEvent(10000,lrcSlot); -- 10000 == EVT_CMD
	lrcSlot:Release();
	math.randomseed(os.time());
	local souiFac = CreateSouiFactory();
	local timerSlot = CreateEventSlot("on_timer");
	runTimer = souiFac:CreateTimer(timerSlot);
	timerSlot:Release();
	souiFac:Release();
end

function on_exit(args)
	slog("execute script function: on_exit");
	win = getHostFromInitEvent(args);
	HostWnd_SetMsgHandler(win,"",nil);--remove msg handler
	runTimer:Release();
end

function on_timer(args)
	if(gamewnd ~= nil) then

		local rcCanvas = gamecanvas:GetWindowRect2();
		local heiCanvas = rcCanvas:Height();
		local widCanvas = rcCanvas:Width();

		local rcPlayer =  players[1]:GetWindowRect2();
		local wid = rcPlayer:Width();
		local hei = rcPlayer:Height();

		local win_id = 0;
		for i = 1,4 do
			local prog = prog_all[i];
			if(prog<prog_max) then
				prog = prog + math.random(0,10);
				prog_all[i] = prog;
				local rc = players[i]:GetWindowRect2();
				rc.left = rcCanvas.left + (widCanvas-wid)*prog/prog_max;
				players[i]:Move2(rc.left,rc.top,-1,-1);
			else
				win_id = i;

				local rc = players[i]:GetWindowRect2();
				rc.left = rcCanvas.left + (widCanvas-wid);
				players[i]:Move2(rc.left,rc.top,-1,-1);
			end
		end

		if win_id ~= 0 then
			gamewnd:FindChildByNameA("btn_run",-1):FireCommand();
			coins_all = coins_all + coins_bet[win_id] * 4;
			gamewnd:FindChildByNameA("txt_coins",-1):SetWindowText(T(coins_all));

			coins_bet = {0,0,0,0};

			local rcPlayer = players[win_id]:GetWindowRect2();
			local widPlayer = rcPlayer:Width();
			local heiPlayer = rcPlayer:Height();
			local szFlag = CSize(0,0); 
			flag_win:GetDesiredSize(szFlag,widPlayer,heiPlayer);
			rcPlayer.right = rcPlayer.left + szFlag.cx;
			rcPlayer.bottom = rcPlayer.top + szFlag.cy;
			rcPlayer:OffsetRect(-szFlag.cx,-szFlag.cy/3);

			flag_win:Move(rcPlayer);
			flag_win:SetVisible(1,1);
			flag_win:SetUserData(win_id);

			for i= 101,104 do
				gamewnd:FindChildByID(i,-1):SetWindowText(T("0"));
			end
		end
	end
end

function on_bet(args)
	if tid ~= 0 then
		return 1;
	end

	local btn = toSWindow(args:Sender());
	if coins_all >= 10 then
	    --id range from 101-104
		id = btn:GetID()-100;
		coins_bet[id] = coins_bet[id] + 10;
		coins_all = coins_all -10;
		btn:SetWindowText(T(coins_bet[id]));

		gamewnd:FindChildByNameA("txt_coins",-1):SetWindowText(T(coins_all));

	end
	return 1;
end

function on_canvas_size(args)
	if win == nil then
		return 0;
	end

	local rcCanvas =  gamecanvas:GetWindowRect2();
	local heiCanvas = rcCanvas:Height();
	local widCanvas = rcCanvas:Width();

	local szPlayer =CSize(0,0);
	players[1]:GetDesiredSize(szPlayer,widCanvas,heiCanvas);

	local wid = szPlayer.cx;
	local hei = szPlayer.cy;

	local rcPlayer = CRect(0,0,wid,hei);
	local interval = (heiCanvas - hei*4)/5;
	rcPlayer:OffsetRect(rcCanvas.left,rcCanvas.top+interval);
	for i = 1, 4 do
		local rc = rcPlayer;
		rc.left = rcCanvas.left + (widCanvas-wid)*prog_all[i]/prog_max;
		rc.right = rc.left+wid;
		players[i]:Move(rc);
		rcPlayer:OffsetRect(0,interval+hei);
	end

	local win_id = flag_win:GetUserData();
	if win_id ~= 0 then
		local rcPlayer = players[win_id]:GetWindowRect2();
		local widPlayer = rcPlayer:Width();
		local heiPlayer = rcPlayer:Height();
		local szFlag = CSize(0,0);
		flag_win:GetDesiredSize(szFlag,widPlayer,heiPlayer);
		flag_win:Move2(rcPlayer.left-szFlag.cx,rcPlayer.top-szFlag.cy/3,-1,-1);
	end

	return 1;

end

function on_run(args)

	if win == nil then
		return 0;
	end
	local btn = toSWindow(args:Sender());
	if tid == 0 then
		prog_all = {0,0,0,0};
		on_canvas_size(nil);
		tid = runTimer:StartTimer(50,1,0);
		btn:SetWindowText(T"stop");
		flag_win:SetVisible(0,1);
	else
		runTimer:KillTimer();
		btn:SetWindowText(T"run");
		tid = 0;
	end
	return 1;
end

function on_btn_select_cbx(args)
	local btn = toSWindow(args:Sender());
	local cbxwnd = btn:GetWindow(2);--get previous sibling
	local cbx = toSComboBase(cbxwnd);
	cbx:SetCurSel(-1);
end
