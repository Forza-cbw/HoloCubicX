#include "app_controller.h"
#include "app_controller_gui.h"
#include "app/app_name.h"
#include "common.h"
#include "interface.h"
#include "Arduino.h"
#include "gui_lock.h"

const char *app_event_type_info[] = {"APP_MESSAGE_WIFI_STA", "APP_MESSAGE_WIFI_ALIVE",
                                     "APP_MESSAGE_WIFI_AP", "APP_MESSAGE_WIFI_DISABLE",
                                     "APP_MESSAGE_WIFI_AP_CLOSE", "APP_MESSAGE_GET_PARAM",
                                     "APP_MESSAGE_SET_PARAM","APP_MESSAGE_READ_CFG",
                                     "APP_MESSAGE_WRITE_CFG","APP_MESSAGE_NONE"};

volatile static bool isRunEventDeal = false;

// TickType_t mainFormRefreshLastTime;
// const TickType_t xDelay500ms = pdMS_TO_TICKS(500);
// mainFormRefreshLastTime = xTaskGetTickCount();
// vTaskDelayUntil(&mainFormRefreshLastTime, xDelay500ms);

void eventDealHandle(TimerHandle_t xTimer)
{
    isRunEventDeal = true;
}

AppController::AppController(const char *name)
{
    strncpy(this->name, name, APP_CONTROLLER_NAME_LEN);
    app_num = 0;
    app_running_flag = 0;
    cur_app_index = 0;
    pre_app_index = 0;
    // appList = new APP_OBJ[APP_MAX_NUM];
    saverDisable = false;  // APP 未屏蔽屏保
    m_saverActive = false; // 屏保未触发
    m_preWifiReqMillis = GET_SYS_MILLIS();
    m_preActionMillis = GET_SYS_MILLIS();

    // 定义一个事件处理定时器
    xTimerEventDeal = xTimerCreate("Event Deal",
                                   300 / portTICK_PERIOD_MS,
                                   pdTRUE, (void *)0, eventDealHandle);
    // 启动事件处理定时器
    xTimerStart(xTimerEventDeal, 0);
}

void AppController::init(void)
{
    // 设置CPU主频
    if (1 == this->sys_cfg.power_mode)
    {
        setCpuFrequencyMhz(240);
    }
    else
    {
        setCpuFrequencyMhz(80);
    }
    // uint32_t freq = getXtalFrequencyMhz(); // In MHz
    log_i("CpuFrequencyMhz: %d",getCpuFrequencyMhz());

    LVGL_OPERATE_LOCK(app_control_gui_init();)
    appList[0] = new APP_OBJ();
    appList[0]->app_image = &app_loading;
    appList[0]->app_name = "Loading...";
    appTypeList[0] = APP_TYPE_REAL_TIME;
    LVGL_OPERATE_LOCK(app_control_display_scr(appList[cur_app_index]->app_image,
                            appList[cur_app_index]->app_name,
                            LV_SCR_LOAD_ANIM_NONE, true);)
}

AppController::~AppController()
{
    rgb_task_del();
}

int AppController::app_is_legal(const APP_OBJ *app_obj)
{
    // APP的合法性检测
    if (NULL == app_obj)
        return 1;
    if (APP_MAX_NUM <= app_num)
        return 2;
    return 0;
}

// 将APP安装到app_controller中
int AppController::app_install(APP_OBJ *app, APP_TYPE app_type)
{
    int ret_code = app_is_legal(app);
    if (0 != ret_code)
    {
        return ret_code;
    }

    appList[app_num] = app;
    appTypeList[app_num] = app_type;
    ++app_num;
    return 0; // 安装成功
}

int AppController::app_auto_start()
{
    // APP自启动
    int index = this->getAppIdxByName(sys_cfg.auto_start_app.c_str());
    if (index < 0)
    {
        // 没找到相关的APP
        return 0;
    }
    // 进入自启动的APP
    app_running_flag = 1; // 进入app, 如果已经在
    cur_app_index = index;
    (*(appList[cur_app_index]->app_init))(this); // 执行APP初始化
    return 0;
}

int AppController::main_process(ImuAction *act_info)
{
    if (ACTIVE_TYPE::UNKNOWN != act_info->active)
    {
        log_i("[Operate]\tact_info->active:%s ",active_type_info[act_info->active]);
        if (m_saverActive) {
            log_i("Screen saver disable,\t set backLight: [%d]", sys_cfg.back_light);
            screen.setBackLight(sys_cfg.back_light / 100.0);
            rgb_resume();
            m_saverActive = false; // 屏保关闭
        }
        m_preActionMillis = GET_SYS_MILLIS();
    }

    if (isRunEventDeal)
    {
        isRunEventDeal = false;
        // 扫描事件
        this->req_event_deal(false); // 事件处理器不要做成后台任务，会有数据一致性的问题
    }

    // wifi自动关闭(在节能模式下)
    if (0 == sys_cfg.power_mode && WIFI_MODE_NULL != WiFi.getMode()
        && doDelayMillisTime(WIFI_LIFE_CYCLE, &m_preWifiReqMillis, false))
    {
        send_to(SELF_SYS_NAME, WIFI_SYS_NAME, APP_MESSAGE_WIFI_DISABLE, 0, NULL);
    }

    // 屏保触发
    if (false == saverDisable &&  0 != sys_cfg.screensaver_interval
        && doDelayMillisTime(sys_cfg.screensaver_interval, &m_preActionMillis, false))
    {
        log_i("Screen saver enable,\t set backLight: [%d]", sys_cfg.back_light2);
        screen.setBackLight(sys_cfg.back_light2 / 100.0);
        rgb_pause();
        m_saverActive = true; // 屏保触发
        m_preActionMillis = GET_SYS_MILLIS();
    }

    if (0 == app_running_flag)
    {
        // 当前没有进入任何app
        lv_scr_load_anim_t anim_type = LV_SCR_LOAD_ANIM_NONE;
        if (ACTIVE_TYPE::TURN_LEFT == act_info->active)
        {
            anim_type = LV_SCR_LOAD_ANIM_MOVE_RIGHT;
            pre_app_index = cur_app_index;
            cur_app_index = (cur_app_index + 1) % app_num;
            log_i("Current App: %s" , appList[cur_app_index]->app_name);
        }
        else if (ACTIVE_TYPE::TURN_RIGHT == act_info->active)
        {
            anim_type = LV_SCR_LOAD_ANIM_MOVE_LEFT;
            pre_app_index = cur_app_index;
            // 以下等效与 processId = (processId - 1 + APP_NUM) % 4;
            // +3为了不让数据溢出成负数，而导致取模逻辑错误
            cur_app_index = (cur_app_index - 1 + app_num) % app_num; // 此处的3与p_processList的长度一致
            log_i("Current App: %s" , appList[cur_app_index]->app_name);
        }
        else if (ACTIVE_TYPE::DOWN_MORE == act_info->active)
        {
            app_running_flag = 1;
            if (NULL != appList[cur_app_index]->app_init)
            {
                (*(appList[cur_app_index]->app_init))(this); // 执行APP初始化
            }
        }

        // 如果不是甩动，则切换菜单显示的图标
        if (ACTIVE_TYPE::DOWN_MORE != act_info->active) // && UNKNOWN != act_info->active
        {
            LVGL_OPERATE_LOCK(app_control_display_scr(appList[cur_app_index]->app_image,
                                    appList[cur_app_index]->app_name,
                                    anim_type, false);)
            vTaskDelay(200 / portTICK_PERIOD_MS);
        }
    }
    else
    {
        // 运行APP主进程一次（主进程中不可以无限循环，主循环在HoloCubic_AIO.cpp中）
        (*(appList[cur_app_index]->main_process))(this, act_info);
    }
    // 姿态值清空
    act_info->active = ACTIVE_TYPE::UNKNOWN;
    return 0;
}

APP_OBJ *AppController::getAppByName(const char *name)
{
    for (int pos = 0; pos < app_num; ++pos)
    {
        if (!strcmp(name, appList[pos]->app_name))
        {
            return appList[pos];
        }
    }

    return NULL;
}

int AppController::getAppIdxByName(const char *name)
{
    for (int pos = 0; pos < app_num; ++pos)
    {
        if (!strcmp(name, appList[pos]->app_name))
        {
            return pos;
        }
    }

    return -1;
}

// 通信中心（消息转发）
int AppController::send_to(const char *from, const char *to,
                           APP_MESSAGE_TYPE type, void *message,
                           void *ext_info, bool isSync)
{
    assert(NULL != from && NULL != to);
    int ret = 0;
    log_i("Add\t [%s]\t -> [%s]\t: [%s]", from, to, app_event_type_info[type]);

    if (eventList.size() > EVENT_LIST_MAX_LENGTH) {
        log_e("Reject\t [%s]\t -> [%s]\t: [%s]", from, to, app_event_type_info[type]);
        ret = 1;
    } else {
        // 插入事件列表
        EVENT_OBJ new_event = {from, to, type, message, ext_info, isSync, 3, 0, 0};
        eventList.push_back(new_event);
        log_i("Add\t [%s]\t -> [%s]\t: [%s]", from, to, app_event_type_info[type]);
    }
    log_i("EventList Size: %d",eventList.size());

    if (isSync) // 同步消息，立刻同步调用事件处理函数
        req_event_deal(true); // isSync==true

    return ret;
}

std::list<EVENT_OBJ>::iterator AppController::event_wait_retry(std::list<EVENT_OBJ>::iterator event) {
    std::list<EVENT_OBJ>::iterator ret;

    event->retryCount += 1;
    if (event->retryCount >= event->retryMaxNum)
    {   // 多次重试失败
        log_e("Failed\t [%d] times", event->retryCount);
        log_i("Delete\t from EventList");
        log_i("EventList Size: %d",eventList.size() - 1);
        return eventList.erase(event); // 删除该响应事件
    }
    else
    {   // 下次重试
        log_w("Failed\t [%d] times and wait for retry", event->retryCount);
        event->nextRunTime = GET_SYS_MILLIS() + 4000;
        return ++event;
    }
}

int AppController::req_event_deal(bool onlySync)
{
    // 请求事件的处理
    for (std::list<EVENT_OBJ>::iterator event = eventList.begin(); event != eventList.end();)
    {
        assert(NULL != event->from && NULL != event->to);
        APP_OBJ *fromApp = getAppByName(event->from); // 来自谁 有可能为空
        APP_OBJ *toApp = getAppByName(event->to);     // 发送给谁 有可能为空
        bool isDone;

        if (event->nextRunTime > GET_SYS_MILLIS()) { // 没到重试时间
            ++event;
            continue;
        }
        if (onlySync && !event->isSync) { // 如果是同步调用，只处理同步任务
            ++event;
            continue;
        }

        log_i("--------------------------------------------------------------------------");
        log_i("Handle\t [%s]\t -> [%s]\t: [%s]", event->from, event->to, app_event_type_info[event->type]);

        // 在这里区分WIFI事件消息 和 其他消息
        if (event->type <= APP_MESSAGE_WIFI_AP_CLOSE) { // WIFI事件消息： 先处理WIFI任务再回调message_handler
            assert(!strcmp(event->to, WIFI_SYS_NAME));
            isDone = wifi_event(event->type);
        } else if (event->type <= APP_MESSAGE_WRITE_CFG) {
            // assert(!strcmp(event->to, CONFIG_SYS_NAME) || !strcmp(event->to, **_APP_NAME))
            if (!strcmp(event->to, CONFIG_SYS_NAME)) { // 读写系统配置（没有回调，toApp==NULL）
                deal_config(event->type, (const char *)event->message, (char *)event->ext_info);
            } // APP_MESSAGE_WRITE_CFG也有可能目的是某个APP，此时会有回调
            isDone = true;
        }
        else { // 其他消息：
            isDone = true;
        }

        // 失败重试机制
        if (!isDone)
        {
            event = event_wait_retry(event);
            continue;
        }

        // 回调
        if (event->type <= APP_MESSAGE_WIFI_AP_CLOSE) // WIFI事件消息： 先处理WIFI任务再回调message_handler
        {
            assert(NULL != fromApp);
            if (NULL != fromApp->message_handle)
            {
                log_i("Callback\t [%s]", fromApp->app_name);
                fromApp->message_handle(SELF_SYS_NAME, fromApp->app_name, // !!!wifi事件结束调用的是from的消息处理函数
                                                 event->type, event->message, NULL);
            }
        } else // 其他消息：
        {
            if (NULL != toApp && NULL != toApp->message_handle) // 发给APP的消息
            {
                log_i("Callback\t [%s]", toApp->app_name);
                toApp->message_handle(event->from, event->to, event->type, event->message, event->ext_info);
            }
        }

        log_i("Delete\t from EventList");
        event = eventList.erase(event); // 删除该响应完成的事件
        log_i("EventList Size: %d",eventList.size());
    }
    return 0;
}

/**
 *  wifi事件的处理
 *  事件处理成功返回true 否则false
 * */
bool AppController::wifi_event(APP_MESSAGE_TYPE type)
{
    bool result = true;
    switch (type)
    {
        case APP_MESSAGE_WIFI_STA:
        {
            g_network.start_conn_wifi(sys_cfg.ssid_0.c_str(), sys_cfg.password_0.c_str()); // 使能wifi-sta连接
            if (!g_network.is_conn_wifi())
                result = false;
        } // 注意这里没有break！！！！
        case APP_MESSAGE_WIFI_ALIVE:
        {   // 持续收到心跳 wifi才不会（因为自己写的省电逻辑）被关闭
            m_preWifiReqMillis = GET_SYS_MILLIS(); // 更新wifi保活时间戳
        }
        break;
        case APP_MESSAGE_WIFI_AP:
        {
            // 更新请求
            g_network.open_ap(AP_SSID);
            m_preWifiReqMillis = GET_SYS_MILLIS();
        }
        break;
        case APP_MESSAGE_WIFI_DISABLE:
        {
            g_network.close_wifi();
            // m_preWifiReqMillis = GET_SYS_MILLIS() - WIFI_LIFE_CYCLE;
        }
        break;
        case APP_MESSAGE_WIFI_AP_CLOSE:
        {
            g_network.close_ap();
        }
        break;
        default:
            break;
    }

    return result;
}

void AppController::app_exit()
{
    app_running_flag = 0; // 退出APP

    // 清空该对象的所有请求
    for (std::list<EVENT_OBJ>::iterator event = eventList.begin(); event != eventList.end();)
    {
        APP_OBJ *fromApp = getAppByName(event->from); // 来自谁 在这里不应该为空，因为一定是某个app调用的本函数
        assert(NULL != fromApp);
        if (appList[cur_app_index] == fromApp)
        {
            event = eventList.erase(event); // 删除该响应事件
        }
        else
        {
            ++event;
        }
    }

    if (NULL != appList[cur_app_index]->exit_callback)
    {
        // 执行APP退出回调
        (*(appList[cur_app_index]->exit_callback))(NULL);
    }
    LVGL_OPERATE_LOCK(app_control_display_scr(appList[cur_app_index]->app_image,
                            appList[cur_app_index]->app_name,
                            LV_SCR_LOAD_ANIM_NONE, true);)

    // 恢复RGB灯  HSV色彩模式
    rgb_task_run(&rgb_cfg);
    // 恢复屏保
    setSaverDisable(false);

    // 设置CPU主频
    if (1 == this->sys_cfg.power_mode)
    {
        setCpuFrequencyMhz(240);
    }
    else
    {
        setCpuFrequencyMhz(80);
    }
    log_i("CpuFrequencyMhz: %d",getCpuFrequencyMhz());
}

void AppController::setSaverDisable(boolean isDisable) {
    this->saverDisable = isDisable;
}
