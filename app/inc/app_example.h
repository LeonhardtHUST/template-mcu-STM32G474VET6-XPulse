/**
  ******************************************************************************
  * @file    app_example.h
  * @brief   应用层起步示例：main.c 的三个 USER CODE 块只转调这里声明的函数。
  *
  * 本目录由使用者自维护，CubeMX 既不生成也不覆盖；目录职责见 docs/LAYOUT.md。
  ******************************************************************************
  */
#ifndef APP_EXAMPLE_H
#define APP_EXAMPLE_H

#ifdef __cplusplus
extern "C" {
#endif

#define APP_EXAMPLE_HUST 1037

/* main.c 的胶水只调用这两个函数，业务逻辑全部留在 app/ 下。 */
void app_init(void);
void app_loop(void);

/* 与 Core/Src/example.c 的 example_to_hust() 并存，名字必须不同：
 * 两份都参与编译，同名会造成链接期 multiple definition。 */
int app_example_to_hust(int a);

#ifdef __cplusplus
}
#endif

#endif /* APP_EXAMPLE_H */
