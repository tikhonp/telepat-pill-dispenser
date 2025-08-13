#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Инициализирует светодиодную ленту WS2812B
 * 
 * @param gpio_num Номер GPIO пина, к которому подключена лента
 * @param led_count Количество светодиодов в ленте
 * @return true если инициализация прошла успешно
 * @return false если произошла ошибка
 */
bool ws2812b_init(uint8_t gpio_num, uint16_t led_count);

/**
 * @brief Устанавливает цвет отдельного светодиода
 * 
 * @param index Индекс светодиода (от 0 до led_count-1)
 * @param r Красный компонент (0-255)
 * @param g Зеленый компонент (0-255)
 * @param b Синий компонент (0-255)
 * @return true если операция прошла успешно
 * @return false если произошла ошибка
 */
bool ws2812b_set_pixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Устанавливает одинаковый цвет для всех светодиодов в ленте
 * 
 * @param r Красный компонент (0-255)
 * @param g Зеленый компонент (0-255)
 * @param b Синий компонент (0-255)
 * @return true если операция прошла успешно
 * @return false если произошла ошибка
 */
bool ws2812b_set_all_pixels(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Выключает все светодиоды (устанавливает цвет RGB(0,0,0))
 * 
 * @return true если операция прошла успешно
 * @return false если произошла ошибка
 */
bool ws2812b_clear_all(void);

/**
 * @brief Обновляет состояние светодиодной ленты
 * Необходимо вызывать после любых изменений цвета светодиодов
 * 
 * @return true если операция прошла успешно
 * @return false если произошла ошибка
 */
bool ws2812b_refresh(void);

/**
 * @brief Деинициализирует светодиодную ленту
 * 
 * @return true если операция прошла успешно
 * @return false если произошла ошибка
 */
bool ws2812b_deinit(void);

/**
 * @brief Получает информацию о том, инициализирована ли лента
 * 
 * @return true если лента инициализирована
 * @return false если лента не инициализирована
 */
bool ws2812b_is_initialized(void);

/**
 * @brief Получает количество светодиодов в ленте
 * 
 * @return Количество светодиодов или 0, если лента не инициализирована
 */
uint16_t ws2812b_get_led_count(void);
