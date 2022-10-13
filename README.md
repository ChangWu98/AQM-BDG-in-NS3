# AQM-BDG-in-NS3
Implementation of AQM Algorithm in 5G-AN RLC Queue Based on NS-3. An end-to-end congestion control scheme includes AQM based RLC queuing delay guarantee

## The contributions of AQM-BDG can be summarized as follows:
`Achieving queuing delay guarantee of RLC queue through AQM algorithm in RLC buffer.
`AQM-BDG extends the granularity of QoS control so that SDFs mapped to the same DRB can obtain the corresponding throughput share according to the preset priority.
`An ECN marking system that can implement source rate regulation through packet header marking.
`AQM-BDG ensures that the RLC buffer queuing delay does not exceed the preset threshold, while almost fully utilizing the allocated resources in highly time-varying wireless links.

## Performance result of the AQM-BDG
Delay guarantee and Link capacity tracking
![amliranStream1Th20ms](https://user-images.githubusercontent.com/90489985/195551530-03ef374d-50c8-4c84-aaa6-d001348fbcd1.png)
![delayCDF](https://user-images.githubusercontent.com/90489985/195551662-23bc7c3d-7e0a-4c86-940a-9e2249f4e356.png)
Multi-stream fairness(priority---stream0:0.5,stream1:1)
![amliranRateStream2](https://user-images.githubusercontent.com/90489985/195552318-83ac1a2c-7190-49a2-8ce4-a049041e11a3.png)
