from ultralytics import YOLO
import torch
from ultralytics.nn.autobackend import check_class_names
import torch.nn as nn
import torchvision
from torchvision.models import resnet18
example_input = torch.randn(1,3,640,640)
model = YOLO('yolov8n.pt')
model.train(data='yolo-lego.yaml',workers=0, epochs=30,batch=16)
model.export(format='torchscript',imgsz=640)
#model_scripted = torch.jit.trace(model,example_input) # Export to TorchScript
#torch.jit.save(model_scripted, 'exported_model.pt')
