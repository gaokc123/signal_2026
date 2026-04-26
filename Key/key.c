/*
void GROUP1_IRQHandler(void)
{
    switch (DL_Interrupt_getPendingGroup(DL_INTERRUPT_GROUP_1)) {
        case Switchs_GPIOA_INT_IIDX:
            if (DL_GPIO_getEnabledInterruptStatus(Switchs_S1_PORT,Switchs_S1_PIN)) {
                DL_GPIO_togglePins(GPIO_LEDS_PORT,GPIO_LEDS_USER_LED_1_PIN);
                DL_GPIO_clearInterruptStatus(Switchs_S1_PORT,Switchs_S1_PIN);
            }else if (DL_GPIO_getEnabledInterruptStatus(Switchs_S3_PORT,Switchs_S3_PIN)) {
                DL_GPIO_togglePins(GPIO_LEDS_PORT,GPIO_LEDS_USER_LED_3_PIN);
                DL_GPIO_clearInterruptStatus(Switchs_S3_PORT,Switchs_S3_PIN);
            }                
            break;
        case Switchs_GPIOB_INT_IIDX:
            DL_GPIO_togglePins(GPIO_LEDS_PORT,GPIO_LEDS_USER_LED_2_PIN);
            DL_GPIO_clearInterruptStatus(Switchs_S2_PORT,Switchs_S2_PIN);
            break;  
    }
}
*/