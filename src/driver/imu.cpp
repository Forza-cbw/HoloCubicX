#include "imu.h"
#include "common.h"

const char *active_type_info[] = {"TURN_RIGHT","TURN_LEFT",
                                  "UP","DOWN",
                                  "DOWN_MORE", "UNKNOWN"};

IMU::IMU()
{
    action_info.active = ACTIVE_TYPE::UNKNOWN;
    action_info.long_time = true;
    action_info.isDownMored = false;
    this->order = 0; // 表示方位
}

void IMU::init(uint8_t order, uint8_t auto_calibration,
               SysMpuConfig *mpu_cfg)
{
    this->setOrder(order); // 设置方向
    Wire.begin(IMU_I2C_SDA, IMU_I2C_SCL);
    Wire.setClock(400000);
    unsigned long timeout = 5000;
    unsigned long preMillis = GET_SYS_MILLIS();
    // mpu = MPU6050(0x68, &Wire);
    // mpu = MPU6050(0x68);
    // while (!mpu.testConnection() && !doDelayMillisTime(timeout, &preMillis, false))
    //     ;

    // if (!mpu.testConnection())
    // {
    //     log_i("Unable to connect to MPU6050.\n");
    //     return;
    // }

    // log_i("Initialization MPU6050 now, Please don't move.\n");
    // mpu.initialize();

    log_i("[IMU] Init QMI8658...\n");
    if (QMI8658_init())
    {
        log_i("[IMU] QMI8658 connection successful\n");
    }
    else
    {
        log_i("[IMU] QMI8658 connection failed\n");
        return;
    }

    // if (auto_calibration == 0)
    // {
    //     // supply your own gyro offsets here, scaled for min sensitivity
    //     mpu.setXGyroOffset(mpu_cfg->x_gyro_offset);
    //     mpu.setYGyroOffset(mpu_cfg->y_gyro_offset);
    //     mpu.setZGyroOffset(mpu_cfg->z_gyro_offset);
    //     mpu.setXAccelOffset(mpu_cfg->x_accel_offset);
    //     mpu.setYAccelOffset(mpu_cfg->y_accel_offset);
    //     mpu.setZAccelOffset(mpu_cfg->z_accel_offset); // 1688 factory default for my test chip
    // }
    // else
    // {
    //     // 启动自动校准
    //     // 7次循环自动校正
    //     mpu.CalibrateAccel(7);
    //     mpu.CalibrateGyro(7);
    //     mpu.PrintActiveOffsets();

    //     mpu_cfg->x_gyro_offset = mpu.getXGyroOffset();
    //     mpu_cfg->y_gyro_offset = mpu.getYGyroOffset();
    //     mpu_cfg->z_gyro_offset = mpu.getZGyroOffset();
    //     mpu_cfg->x_accel_offset = mpu.getXAccelOffset();
    //     mpu_cfg->y_accel_offset = mpu.getYAccelOffset();
    //     mpu_cfg->z_accel_offset = mpu.getZAccelOffset();
    // }


}

void IMU::setOrder(uint8_t order) // 设置方向
{
    this->order = order; // 表示方位
}

ImuAction *IMU::getAction(void)
{
    // 基本方法: 通过对近来的动作数据简单的分析，确定出动作的类型
    ImuAction tmp_info;
    getVirtureMotion6(&tmp_info);

    tmp_info.active = ACTIVE_TYPE::UNKNOWN;
    // todo 原先判断的只是加速度，现在要加上陀螺仪
    // 优先判定大幅down动作
    if (tmp_info.v_ax < -600) {
        tmp_info.active = ACTIVE_TYPE::DOWN_MORE;
    }
    // 左右倾斜
    else if (tmp_info.v_ay < - 200)
    {
        tmp_info.active = ACTIVE_TYPE::TURN_LEFT;
    }
    else if (tmp_info.v_ay > 200)
    {
        tmp_info.active = ACTIVE_TYPE::TURN_RIGHT;
    }
    // 前后倾斜
    else if (tmp_info.v_ax > 500)
    {
        tmp_info.active = ACTIVE_TYPE::UP;
    }
    else if (tmp_info.v_ax < 100)
    {
        tmp_info.active = ACTIVE_TYPE::DOWN;
    }

    // 防止重复检测DOWN_MORE
    if (ACTIVE_TYPE::DOWN_MORE != tmp_info.active) {
        action_info.isDownMored = false;
    }
    else {
        if (action_info.isDownMored) tmp_info.active = ACTIVE_TYPE::UNKNOWN;
        action_info.isDownMored = true;
    }
    action_info.active = tmp_info.active;

    return &action_info;
}

void IMU::getVirtureMotion6(ImuAction *action_info)
{
    float acc[3], gyro[3];
    unsigned int tim_count = 0;
            
    QMI8658_read_xyz(acc, gyro, &tim_count);
    action_info->v_ay = acc[0];
    action_info->v_ax = acc[1];
    action_info->v_az = acc[2];

    action_info->v_gx = gyro[0];
    action_info->v_gy = gyro[1];
    action_info->v_gz = gyro[2];

    log_i("a=%d,%d,%d,g=%d,%d,%d",action_info->v_ax,action_info->v_ay,action_info->v_az,action_info->v_gx,action_info->v_gy,action_info->v_gz);

    if (order & X_DIR_TYPE)
    {
        action_info->v_ax = -action_info->v_ax;
        action_info->v_gx = -action_info->v_gx;
    }

    if (order & Y_DIR_TYPE)
    {
        action_info->v_ay = -action_info->v_ay;
        action_info->v_gy = -action_info->v_gy;
    }

    if (order & Z_DIR_TYPE)
    {
        action_info->v_az = -action_info->v_az;
        action_info->v_gz = -action_info->v_gz;
    }

    if (order & XY_DIR_TYPE)
    {
        int16_t swap_tmp;
        swap_tmp = action_info->v_ax;
        action_info->v_ax = action_info->v_ay;
        action_info->v_ay = swap_tmp;
        swap_tmp = action_info->v_gx;
        action_info->v_gx = action_info->v_gy;
        action_info->v_gy = swap_tmp;
    }
}