import express from "express";
import { receiveSensorData } from "../controllers/sensorController.js";

const router = express.Router();

// POST endpoint that sensor hardware will call
router.post("/data", receiveSensorData);

export default router;
