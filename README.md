# High_Dynamic_Range_Imaging
A program to generate HDR image from several images with different exposure times.

# Instruction

1.	將要處理的多張影像直接放到跟執行檔同目錄，並由曝光短至曝光長命名成 名字 + 0~n + .jpg。(EX. 名字為DSC，則照片命名為DSC0.jpg, DSC1.jpg…以此類推)
2.	執行主程式，輸入要處理的照片數. 名字(不用加上數字編號)以及副檔名。
3.	等待執行(此步驟會有點久)
4.	執行完畢後會跳出處理完的影像視窗，處理好的照片會直接生成在執行檔的目錄底下，其中有bilateral, reinhard, .exr，以及Response Curve的數值會輸出到.csv，其中三列數值分別代表R, G, B的Response Curve。
